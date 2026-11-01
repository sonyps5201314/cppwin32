#pragma once

namespace cppwin32
{
    conditional_native_alias const* find_conditional_alias(std::string_view ns, std::string_view name)
    {
        auto found = conditional_alias_indices.find({ns, std::string(name)});
        return found == conditional_alias_indices.end() ? nullptr : &conditional_native_aliases[found->second.front()];
    }

    bool is_conditional_alias(TypeDef const& type)
    {
        if (!IsCppTypedef(type)) return false;
        auto found = conditional_alias_indices.find({type.TypeNamespace(), std::string(type.TypeDisplayName())});
        if (found == conditional_alias_indices.end()) return false;
        std::set<std::string> targets;
        for (auto index : found->second) targets.insert(conditional_native_aliases[index].target);
        return targets.size() > 1;
    }

    bool is_conditional_record_alias(TypeDef const& type, std::string_view name)
    {
        auto found = conditional_alias_indices.find({type.TypeNamespace(), std::string(name)});
        if (found == conditional_alias_indices.end()) return false;
        std::set<std::string> targets;
        for (auto index : found->second) targets.insert(conditional_native_aliases[index].target);
        return targets.size() > 1;
    }

    std::vector<TypeDef> conditional_field_dependencies(Field const& field, Architecture arches)
    {
        auto signature = field.Signature();
        if (signature.Type().ptr_count()) return {};
        auto index = std::get_if<coded_index<TypeDefOrRef>>(&signature.Type().Type());
        auto type = index ? find(*index, arches) : TypeDef{};
        if (!type) return {};
        auto ns = type.TypeNamespace();
        auto name = type.TypeDisplayName();
        if (auto reference = get_attribute(field, "Windows.Win32.Foundation.Metadata", "NativeRecordReferenceAttribute"))
        {
            auto value = reference.Value();
            name = std::get<std::string_view>(std::get<ElemSig>(value.FixedArgs()[0].value).value);
        }
        auto physical = type;
        std::set<TypeDef> visited;
        while (IsCppTypedef(physical) && visited.insert(physical).second)
        {
            auto fields = physical.FieldList();
            if (size(fields) != 1) return {};
            auto value = fields.first.Signature();
            if (value.Type().ptr_count()) return {};
            physical = record_typedef_target(physical);
            if (!physical) break;
        }
        std::vector<TypeDef> result;
        auto found = conditional_alias_indices.find({ns, std::string(name)});
        if (found != conditional_alias_indices.end())
            for (auto index : found->second)
            {
                auto owner = conditional_native_aliases[index].owner;
                if (std::find(result.begin(), result.end(), owner) == result.end()) result.push_back(owner);
            }
        return result;
    }

    void initialize_conditional_aliases(cache const& metadata)
    {
        std::map<std::string, TypeDef> owners;
        std::set<database const*> databases;
        for (auto const& entry : metadata.namespaces())
            for (auto const& item : entry.second.types)
            {
                databases.insert(&item.second.get_database());
                auto origin = get_native_record_origin(item.second);
                if (!origin.tag.empty()) owners.emplace(std::string(origin.tag), item.second);
            }
        std::set<std::tuple<std::string, std::string, std::string, std::string, std::string>> unique;
        for (auto db : databases)
            for (auto const& attribute : db->Assembly[0].CustomAttribute())
            {
                auto name = attribute.TypeNamespaceAndName();
                if (name.first != "Windows.Win32.Foundation.Metadata" || name.second != "NativeConditionalAliasAttribute") continue;
                auto value = attribute.Value();
                auto condition = native_declaration_catalog::argument(value, 0);
                auto kind = native_declaration_catalog::argument(value, 1);
                auto alias = native_declaration_catalog::argument(value, 2);
                auto target = native_declaration_catalog::argument(value, 3);
                auto owner = owners.find(native_declaration_catalog::argument(value, 4));
                if (owner == owners.end()) continue;
                if (!unique.emplace(condition, kind, alias, target, std::string(owner->second.TypeNamespace())).second) continue;
                conditional_native_aliases.push_back({ condition, kind, alias, target, owner->second, "" });
            }
        std::map<std::string_view, std::vector<TypeDef>> names;
        for (auto const& entry : metadata.namespaces())
            for (auto const& item : entry.second.types) names[get_cpp_type_name(item.second)].push_back(item.second);
        for (size_t index = 0; index < conditional_native_aliases.size(); ++index)
        {
            auto& alias = conditional_native_aliases[index];
            conditional_alias_indices[{alias.owner.TypeNamespace(), alias.name}].push_back(index);
            if (alias.kind != "typedef") continue;
            auto found = names.find(alias.name);
            if (found != names.end())
                for (auto const& type : found->second)
                    if (type.TypeNamespace() != alias.owner.TypeNamespace() && IsHiddenTypeNamespace(type)) alias.scope = std::string(alias.owner.TypeNamespace());
        }
    }

}
