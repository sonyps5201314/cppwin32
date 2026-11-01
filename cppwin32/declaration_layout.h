#pragma once

namespace cppwin32
{
    using native_owner_key = std::pair<std::string_view, std::string_view>;
    std::map<native_owner_key, std::vector<TypeDef>> related_typedefs;
    std::map<native_owner_key, std::vector<std::string>> related_native_aliases;
    std::map<std::string_view, std::vector<std::string>> namespace_native_aliases;
    std::map<std::string, std::string_view> native_alias_namespaces;
    std::map<std::string_view, std::set<std::string_view>> late_macro_consumers;
    std::map<native_owner_key, std::vector<std::pair<std::string_view, std::string>>> owner_conditionals;
    std::map<std::string_view, std::vector<std::pair<TypeDef, std::pair<std::string_view, std::string_view>>>> late_record_macros;
    std::set<TypeDef> tag_macro_owners;

    void write_conditional_group(writer& w, std::pair<std::string_view, std::string> const& key);

    TypeDef typedef_record_owner(TypeDef const& type)
    {
        if (!IsCppTypedef(type) || is_conditional_alias(type) || nested_typedef_target(type) || is_record_tag_typedef(type)) return {};
        auto fields = type.FieldList();
        if (size(fields) != 1) return {};
        auto signature = fields.first.Signature();
        auto index = std::get_if<coded_index<TypeDefOrRef>>(&signature.Type().Type());
        if (!index) return {};
        auto target = find(*index, GetSupportedArchitectures(type));
        return target && get_native_record_origin(target).present && !is_nested(target) ? target : TypeDef{};
    }

    bool is_late_conditional_alias(std::string_view ns, std::string_view name)
    {
        auto found = conditional_alias_indices.find({ns, std::string(name)});
        if (found == conditional_alias_indices.end()) return false;
        std::set<std::string> targets;
        for (auto index : found->second) targets.insert(conditional_native_aliases[index].target);
        if (targets.size() != 1 || conditional_native_aliases[found->second.front()].kind != "macro") return false;
        auto native = native_declarations.aliases.find(std::string(name));
        if (native != native_declarations.aliases.end() && native->second.primary != *targets.begin()) return true;
        auto known = native_declarations.known.find(std::string(name));
        if (known == native_declarations.known.end()) return false;
        auto origin = get_native_record_origin(known->second);
        return get_cpp_type_name(known->second) == name || std::find(origin.typedefs.begin(), origin.typedefs.end(), name) != origin.typedefs.end();
    }

    void initialize_declaration_layout(cache const& metadata)
    {
        for (auto const& entry : metadata.namespaces())
            for (auto const& item : entry.second.types)
            {
                auto origin = get_native_record_origin(item.second);
                for (auto const& macro : origin.macros)
                    if (macro.first == origin.tag)
                    {
                        tag_macro_owners.insert(item.second);
                        late_record_macros[entry.first].push_back({item.second, macro});
                    }
            }
        for (auto const& entry : conditional_alias_indices)
        {
            if (is_late_conditional_alias(entry.first.first, entry.first.second)) continue;
            std::set<native_owner_key> owners;
            for (auto index : entry.second)
            {
                auto const& owner = conditional_native_aliases[index].owner;
                owners.emplace(owner.TypeNamespace(), owner.TypeDisplayName());
            }
            for (auto const& owner : owners) owner_conditionals[owner].push_back(entry.first);
        }
        std::map<std::string, std::string_view> uses;
        auto tag_consumer = [&](TypeDef type, std::string_view ns)
        {
            if (auto owner = typedef_record_owner(type)) type = owner;
            if (tag_macro_owners.count(type)) late_macro_consumers[type.TypeNamespace()].insert(ns);
        };
        auto record_use = [&](auto const& row, std::string_view ns)
        {
            auto attribute = get_attribute(row, "Windows.Win32.Foundation.Metadata", "NativeDeclaratorAttribute");
            if (!attribute) return;
            auto value = attribute.Value();
            for (auto const& name : native_identifiers(native_declaration_catalog::argument(value, 0))) uses.emplace(name, ns);
        };
        for (auto const& entry : metadata.namespaces())
            for (auto const& item : entry.second.types)
            {
                auto type = item.second;
                if (auto owner = typedef_record_owner(type)) related_typedefs[{owner.TypeNamespace(), owner.TypeDisplayName()}].push_back(type);
                for (auto const& field : type.FieldList())
                {
                    record_use(field, type.TypeNamespace());
                    auto signature = field.Signature();
                    if (auto index = std::get_if<coded_index<TypeDefOrRef>>(&signature.Type().Type()))
                        if (auto target = find(*index, GetSupportedArchitectures(type))) tag_consumer(target, type.TypeNamespace());
                }
                for (auto const& method : type.MethodList())
                {
                    method_signature signature{method};
                    auto inspect = [&](TypeSig const& signature)
                    {
                        auto index = std::get_if<coded_index<TypeDefOrRef>>(&signature.Type());
                        auto target = index ? find(*index, GetSupportedArchitectures(type)) : TypeDef{};
                        if (target) tag_consumer(target, type.TypeNamespace());
                        if (!target || !is_late_conditional_alias(target.TypeNamespace(), target.TypeDisplayName())) return;
                        late_macro_consumers[target.TypeNamespace()].insert(type.TypeNamespace());
                    };
                    if (signature.return_signature()) inspect(signature.return_signature().Type());
                    for (auto const& parameter : signature.params()) inspect(parameter.second->Type());
                    for (auto const& parameter : method.ParamList()) record_use(parameter, type.TypeNamespace());
                }
            }
        for (auto const& alias : conditional_native_aliases)
        {
            uses.emplace(alias.target, alias.owner.TypeNamespace());
            uses.emplace(alias.name, alias.owner.TypeNamespace());
        }
        std::vector<std::pair<std::string, std::string_view>> pending(uses.begin(), uses.end());
        for (size_t index = 0; index < pending.size(); ++index)
        {
            auto const entry = pending[index];
            auto source = native_declarations.aliases.find(entry.first);
            if (source == native_declarations.aliases.end()) continue;
            for (auto const& dependency : native_identifiers(source->second.primary))
                if (uses.emplace(dependency, entry.second).second) pending.emplace_back(dependency, entry.second);
        }
        for (auto const& entry : native_declarations.required)
        {
            std::set<TypeDef> owners;
            auto source = native_declarations.aliases.find(entry.first);
            auto canonical = source == native_declarations.aliases.end() ? entry.second : source->second.canonical;
            for (auto const& name : native_identifiers(canonical))
            {
                auto known = native_declarations.known.find(name);
                if (known == native_declarations.known.end()) continue;
                auto type = known->second;
                if (auto owner = typedef_record_owner(type)) type = owner;
                if (get_native_record_origin(type).present && !is_nested(type)) owners.insert(type);
            }
            if (owners.size() == 1)
            {
                auto owner = *owners.begin();
                native_alias_namespaces.emplace(entry.first, owner.TypeNamespace());
                related_native_aliases[{owner.TypeNamespace(), owner.TypeDisplayName()}].push_back(entry.first);
            }
            else
            {
                auto use = uses.find(entry.first);
                std::string_view ns = use == uses.end() ? std::string_view{} : use->second;
                if (ns.empty())
                    for (auto const& alias : conditional_native_aliases)
                        if (alias.target == entry.first) { ns = alias.owner.TypeNamespace(); break; }
                if (ns.empty()) throw std::invalid_argument("Native typedef has no declaration owner: " + entry.first);
                native_alias_namespaces.emplace(entry.first, ns);
                namespace_native_aliases[ns].push_back(entry.first);
            }
        }
    }

    void write_native_alias(writer& w, std::string const& name)
    {
        if (w.native_aliases_written.count({name, Architecture::All}) || w.native_aliases_written.count({name, w.current_architecture})) return;
        if (!w.native_aliases_pending.insert(name).second) return;
        auto found = native_declarations.required.find(name);
        if (found == native_declarations.required.end()) { w.native_aliases_pending.erase(name); return; }
        for (auto const& dependency : native_identifiers(found->second))
        {
            if (dependency == name) continue;
            if (native_declarations.required.count(dependency)) write_native_alias(w, dependency);
            auto known = native_declarations.known.find(dependency);
            if (known != native_declarations.known.end()) write_native_forward(w, known->second);
            else if (auto tag = native_declarations.tags.find(dependency); tag != native_declarations.tags.end()) w.write("% %;\n", tag->second, tag->first);
        }
        w.write("typedef %;\n", native_declaration(found->second, name));
        w.native_aliases_pending.erase(name);
        if (w.current_architecture != Architecture::None) w.native_aliases_written.emplace(name, w.current_architecture);
    }

    void add_native_dependencies(writer& w, std::string const& declaration)
    {
        for (auto const& name : native_identifiers(declaration))
            if (auto found = native_alias_namespaces.find(name); found != native_alias_namespaces.end()) w.native_namespace_dependencies.insert(found->second);
    }

    void write_native_forward(writer& w, TypeDef const& type)
    {
        if (is_conditional_alias(type))
        {
            write_conditional_group(w, {type.TypeNamespace(), std::string(type.TypeDisplayName())});
            return;
        }
        auto const& ns = type.get_cache().namespaces().at(type.TypeNamespace());
        auto variants = ns.types_same_name.find(type.TypeDisplayName());
        if (variants == ns.types_same_name.end()) write_forward(w, type);
        else for (auto const& variant : variants->second) write_forward(w, variant);
    }

    void write_related_aliases(writer& w, TypeDef const& type, bool definition)
    {
        native_owner_key key{type.TypeNamespace(), type.TypeDisplayName()};
        if (auto found = related_typedefs.find(key); found != related_typedefs.end())
            for (auto const& alias : found->second)
            {
                auto owner_arch = GetSupportedArchitectures(type), alias_arch = GetSupportedArchitectures(alias);
                if (owner_arch != Architecture::None && alias_arch != Architecture::None && !(owner_arch & alias_arch)) continue;
                if (definition)
                {
                    w.related_typedefs_written.erase({alias, Architecture::All});
                    w.related_typedefs_written.erase({alias, w.current_architecture});
                }
                if (w.related_typedefs_written.count({alias, Architecture::All}) || w.related_typedefs_written.count({alias, w.current_architecture})) continue;
                if (!w.related_typedefs_pending.insert(alias).second) continue;
                auto arches = GetSupportedArchitectures(alias);
                check_for_write_defined_arches__part_head(w, arches);
                w.WriteCppTypedef(alias);
                check_for_write_defined_arches__part_tail(w, arches);
                w.related_typedefs_pending.erase(alias);
                if (w.current_architecture != Architecture::None) w.related_typedefs_written.emplace(alias, w.current_architecture);
            }
        if (auto found = related_native_aliases.find(key); found != related_native_aliases.end())
            for (auto const& alias : found->second)
            {
                if (definition)
                {
                    w.native_aliases_written.erase({alias, Architecture::All});
                    w.native_aliases_written.erase({alias, w.current_architecture});
                }
                write_native_alias(w, alias);
            }
    }

    void write_conditional_groups(writer& w, std::vector<std::pair<std::string_view, std::string>> const& keys, bool prepare = true)
    {
        std::vector<size_t> indices;
        for (auto const& key : keys)
        {
            auto state = std::make_tuple(key.first, key.second, w.current_architecture);
            if (w.conditional_aliases_written.count(std::make_tuple(key.first, key.second, Architecture::All)) || !w.conditional_aliases_written.insert(state).second) continue;
            auto const& entries = conditional_alias_indices.at(key);
            indices.insert(indices.end(), entries.begin(), entries.end());
        }
        if (prepare)
            for (auto index : indices)
            {
                auto const& alias = conditional_native_aliases[index];
                write_native_forward(w, alias.owner);
                write_native_alias(w, alias.target);
                auto known = native_declarations.known.find(alias.target);
                if (known != native_declarations.known.end()) write_native_forward(w, known->second);
            }
        std::map<std::string, std::vector<conditional_native_alias const*>> groups;
        for (auto index : indices)
        {
            auto const& alias = conditional_native_aliases[index];
            groups[alias.condition].push_back(&alias);
        }
        while (!groups.empty())
        {
            auto group = std::find_if(groups.begin(), groups.end(), [](auto const& item) { return item.first.find("!defined(UNICODE)") == std::string::npos && item.first.find("!defined(_UNICODE)") == std::string::npos; });
            if (group == groups.end()) group = groups.begin();
            auto write = [&](auto const& entries)
            {
                for (auto alias : entries)
                    if (alias->kind == "macro") w.write("#define % %\n", alias->name, alias->target);
                    else
                    {
                        auto guard = wrap_type_namespace(w, alias->scope, alias->scope.empty());
                        w.write("typedef % %;\n", alias->target, alias->name);
                    }
            };
            auto condition = group->first;
            static std::regex const nested(R"(^(.*) && \(defined\((UNICODE|_UNICODE)\)\)$)");
            std::smatch selector;
            if (std::regex_match(condition, selector, nested))
            {
                auto opposite = groups.find(selector[1].str() + " && (!defined(" + selector[2].str() + "))");
                if (opposite != groups.end())
                {
                    w.write("#if %\n#ifdef %\n", selector[1].str(), selector[2].str());
                    write(group->second);
                    w.write("#else\n");
                    write(opposite->second);
                    w.write("#endif\n#endif\n");
                    groups.erase(opposite);
                    groups.erase(group);
                    continue;
                }
            }
            if (condition == "defined(UNICODE)") w.write("#ifdef UNICODE\n");
            else if (condition == "defined(_UNICODE)") w.write("#ifdef _UNICODE\n");
            else w.write("#if %\n", condition);
            write(group->second);
            auto opposite = groups.find(condition.rfind("defined(", 0) == 0 ? "!" + condition : "!(" + condition + ")");
            if (opposite != groups.end() && opposite != group)
            {
                w.write("#else\n");
                write(opposite->second);
                groups.erase(opposite);
            }
            w.write("#endif\n");
            groups.erase(group);
        }
    }

    void write_conditional_group(writer& w, std::pair<std::string_view, std::string> const& key)
    {
        write_conditional_groups(w, {key});
    }

    void write_owner_conditionals(writer& w, TypeDef const& type)
    {
        auto found = owner_conditionals.find({type.TypeNamespace(), type.TypeDisplayName()});
        if (found != owner_conditionals.end()) write_conditional_groups(w, found->second);
    }

    void write_configuration_constants(writer& w, cache::namespace_members const& members)
    {
        auto guard = wrap_type_namespace(w, w.type_namespace);
        for (auto const& type : members.classes)
            for (auto const& field : type.FieldList())
                if (field.Flags().Literal() && is_configuration_constant(field.Name()))
                    w.write("#ifndef %\n    inline constexpr % % = %;\n#endif\n", field.Name(), field.Constant().Type(), field.Name(), field.Constant());
    }

    void write_base_header(cache const& metadata)
    {
        auto source = file_to_string("base.h");
        std::string_view marker = "#pragma region configuration\n";
        auto position = source.find(marker);
        if (position == std::string::npos) throw std::invalid_argument("Missing base configuration insertion point");
        position += marker.size();
        writer output;
        output.write(source.substr(0, position));
        for (auto const& entry : metadata.namespaces())
        {
            bool configured = std::any_of(entry.second.classes.begin(), entry.second.classes.end(), [](auto const& type)
            {
                for (auto const& field : type.FieldList()) if (field.Flags().Literal() && is_configuration_constant(field.Name())) return true;
                return false;
            });
            if (!configured) continue;
            output.type_namespace = entry.first;
            write_configuration_constants(output, entry.second);
        }
        std::map<std::string_view, Constant> constants;
        for (auto const& entry : metadata.namespaces())
            for (auto const& type : entry.second.classes)
                for (auto const& field : type.FieldList())
                    if (field.Flags().Literal() && is_configuration_constant(field.Name()) && field.Name() != "WINVER" && field.Name() != "NTDDI_VERSION" && !ends_with(field.Name(), "_MAXVER")) constants.emplace(field.Name(), field.Constant());
        for (auto const& entry : constants) output.write("#ifndef %\n#define % %\n#endif\n", entry.first, entry.first, entry.second);
        output.write(source.substr(position));
        output.flush_to_file(settings.output_folder + "win32/base.h");
    }

    void write_dependency_forwards(writer& w)
    {
        std::set<TypeDef> written;
        while (true)
        {
            std::vector<TypeDef> pending;
            for (auto const& entry : w.depends)
                for (auto const& type : entry.second)
                    if (written.insert(type).second) pending.push_back(type);
            if (pending.empty()) break;
            for (auto const& type : pending) write_forward(w, type);
        }
    }
}
