#pragma once
#include <regex>

namespace cppwin32
{
    inline std::vector<std::string> native_identifiers(std::string const& text)
    {
        static std::regex const expression(R"(\b[A-Za-z_]\w*\b)");
        std::vector<std::string> result;
        for (std::sregex_iterator i(text.begin(), text.end(), expression), end; i != end; ++i) result.push_back(i->str());
        return result;
    }

    inline std::string native_declaration(std::string type, std::string const& name)
    {
        auto position = type.find("{0}");
        if (position == std::string::npos) return type + " " + name;
        type.replace(position, 3, name);
        return type;
    }

    struct native_declaration_catalog
    {
        struct alias { std::string primary; std::string canonical; };
        std::map<std::string, alias> aliases;
        std::map<std::string, TypeDef> known;
        std::map<std::string, std::string> required;
        std::map<std::string, std::string> tags;
        std::set<TypeDef> forward_dependencies;
        bool active = false;
        std::map<MethodDef, uint16_t> imports;
        std::map<MethodDef, std::pair<std::string_view, std::string_view>> import_names;
        std::map<std::string_view, MethodDef> first_imports;
        std::map<MethodDef, std::string> renamed_imports;

        static std::string argument(CustomAttributeSig const& signature, size_t index)
        {
            return std::string(std::get<std::string_view>(std::get<ElemSig>(signature.FixedArgs()[index].value).value));
        }

        void initialize(cache const& metadata)
        {
            std::set<database const*> databases;
            for (auto const& entry : metadata.namespaces())
                for (auto const& item : entry.second.types)
                {
                    auto const& type = item.second;
                    databases.insert(&type.get_database());
                    known.emplace(std::string(get_cpp_type_name(type)), type);
                    auto origin = get_native_record_origin(type);
                    if (!origin.tag.empty()) known.emplace(std::string(origin.tag), type);
                    for (auto name : origin.typedefs) known.emplace(std::string(name), type);
                    for (auto const& macro : origin.macros) known.emplace(std::string(macro.first), type);
                }
            std::set<std::string> needed;
            auto scan = [&](auto const& row)
            {
                auto attribute = get_attribute(row, "Windows.Win32.Foundation.Metadata", "NativeDeclaratorAttribute");
                if (!attribute) return;
                active = true;
                for (auto const& name : native_identifiers(argument(attribute.Value(), 0))) needed.insert(name);
            };
            for (auto db : databases)
            {
                for (auto const& attribute : db->Assembly[0].CustomAttribute())
                {
                    auto const name = attribute.TypeNamespaceAndName();
                    if (name.first != "Windows.Win32.Foundation.Metadata" || name.second != "NativeAliasAttribute") continue;
                    auto signature = attribute.Value();
                    aliases.emplace(argument(signature, 0), alias{ argument(signature, 1), argument(signature, 2) });
                }
                for (auto const& import : db->ImplMap)
                {
                    auto forwarded = import.get_value<uint32_t>(1);
                    if ((forwarded & 1) != 0)
                    {
                        auto method = db->MethodDef[(forwarded >> 1) - 1];
                        imports.emplace(method, import.get_value<uint16_t>(0));
                        import_names.emplace(method, std::make_pair(db->ModuleRef[import.get_value<uint32_t>(3) - 1].Name(), import.ImportName()));
                    }
                }
                for (auto const& field : db->Field) scan(field);
                for (auto const& param : db->Param) scan(param);
            }
            auto module_name = [](std::string_view name)
            {
                std::string result(name);
                std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return result;
            };
            for (auto const& entry : metadata.namespaces())
                for (auto const& type : entry.second.classes)
                    for (auto const& method : type.MethodList())
                    {
                        if (method.Flags().Access() != MemberAccess::Public) continue;
                        auto import = import_names.find(method);
                        if (import == import_names.end()) continue;
                        auto first = first_imports.emplace(method.Name(), method);
                        if (first.second) continue;
                        auto const& previous = import_names.at(first.first->second);
                        if (module_name(previous.first) == module_name(import->second.first) && previous.second == import->second.second) continue;
                        auto prefix = std::filesystem::path(import->second.first).stem().string();
                        for (auto& c : prefix) if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') c = '_';
                        if (prefix.empty() || std::isdigit(static_cast<unsigned char>(prefix.front()))) prefix.insert(prefix.begin(), '_');
                        renamed_imports.emplace(method, prefix + "_" + std::string(method.Name()));
                    }
            std::set<std::string> visited;
            for (auto const& name : needed) require(name, visited);
        }

        void require(std::string const& name, std::set<std::string>& visited, bool for_alias = false, bool original = false)
        {
            static std::set<std::string> const keywords = {
                "void", "bool", "char", "short", "int", "long", "float", "double", "signed", "unsigned", "const", "volatile",
                "struct", "class", "enum", "union", "wchar_t", "char16_t", "char32_t", "__int8", "__int16", "__int32", "__int64",
                "__cdecl", "__stdcall", "__fastcall", "__vectorcall", "__thiscall", "__restrict", "__unaligned", "__ptr32", "__ptr64",
                "size_t", "intptr_t", "WCHAR", "GUID", "PCSTR", "PCWSTR", "noexcept"
            };
            if (keywords.count(name) || tags.count(name)) return;
            if (!original && std::any_of(conditional_native_aliases.begin(), conditional_native_aliases.end(), [&](auto const& alias) { return alias.name == name; })) return;
            auto existing = known.find(name);
            if (existing != known.end())
            {
                if (for_alias)
                {
                    auto const& namespaces = existing->second.get_cache().namespaces();
                    auto entry = namespaces.find(existing->second.TypeNamespace());
                    auto variants = entry->second.types_same_name.find(existing->second.TypeDisplayName());
                    if (variants == entry->second.types_same_name.end()) forward_dependencies.insert(existing->second);
                    else forward_dependencies.insert(variants->second.begin(), variants->second.end());
                }
                auto origin = get_native_record_origin(existing->second);
                if (origin.present && origin.tag == name) tags.emplace(name, is_union(existing->second) ? "union" : "struct");
                return;
            }
            if (!visited.insert(name).second) return;
            auto found = aliases.find(name);
            if (found == aliases.end()) throw std::invalid_argument("Unresolved native declaration type: " + name);
            auto declaration = for_alias ? found->second.primary : found->second.canonical;
            auto primary = found->second.primary;
            bool platform_sized = false;
            for (auto const& dependency : native_identifiers(primary))
            {
                auto type = known.find(dependency);
                if (type == known.end() || !IsCppTypedef(type->second)) continue;
                for (auto const& field : type->second.FieldList())
                {
                    auto signature = field.Signature();
                    if (auto value = std::get_if<ElementType>(&signature.Type().Type()))
                        if (*value == ElementType::I || *value == ElementType::U)
                        {
                            primary = std::regex_replace(primary, std::regex("\\b" + dependency + "\\b"), *value == ElementType::I ? "intptr_t" : "size_t");
                            platform_sized = true;
                        }
                }
            }
            if (platform_sized) declaration = primary;
            static std::regex const record(R"(\b(struct|class|union)\s+([A-Za-z_]\w*))");
            for (std::sregex_iterator i(declaration.begin(), declaration.end(), record), end; i != end; ++i) tags.emplace((*i)[2].str(), (*i)[1].str());
            for (auto const& dependency : native_identifiers(declaration)) require(dependency, visited, true);
            required.insert_or_assign(name, declaration);
        }

    };

    inline std::string_view native_calling_convention(int convention)
    {
        switch (convention)
        {
        case 0:
        case 1:
        case 3: return "__stdcall";
        case 2: return "__cdecl";
        case 4: return "__thiscall";
        case 5: return "__fastcall";
        default: throw std::invalid_argument("Unsupported native calling convention");
        }
    }

    inline std::string_view delegate_calling_convention(TypeDef const& type)
    {
        auto attribute = get_attribute(type, "System.Runtime.InteropServices", "UnmanagedFunctionPointerAttribute");
        if (!attribute) return "__stdcall";
        auto blob = attribute.get_database().get_blob(attribute.get_value<uint32_t>(2));
        if (blob.as<uint16_t>() != 1) throw std::invalid_argument("Invalid unmanaged calling convention attribute");
        return native_calling_convention(blob.as<int32_t>(2));
    }

    inline native_declaration_catalog native_declarations;
}

namespace cppwin32
{
    inline std::string_view method_calling_convention(MethodDef const& method)
    {
        if (method.Signature().CallConvention() == CallingConvention::VarArg) return "__cdecl";
        auto found = native_declarations.imports.find(method);
        if (found != native_declarations.imports.end()) return native_calling_convention((found->second & 0x700) >> 8);
        return "__stdcall";
    }
}
