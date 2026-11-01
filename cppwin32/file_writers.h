#pragma once

namespace cppwin32
{
    static void write_namespace_0_h(std::string_view const& ns, cache::namespace_members const& members)
    {
        writer w;
        w.type_namespace = ns;

        {
            w.write("#pragma region enums\n");
            w.write_each<write_enum>(members.enums);
            w.write("#pragma endregion enums\n\n");

            w.write("#pragma region forward_declarations\n");
            w.write_each<write_forward>(members.structs);
            w.write_each<write_forward>(members.interfaces);
            for (auto const& type : members.structs)
                if (get_native_record_origin(type).present)
                {
                    write_related_aliases(w, type);
                    write_owner_conditionals(w, type);
                }
            if (auto found = namespace_native_aliases.find(ns); found != namespace_native_aliases.end())
                for (auto const& alias : found->second) write_native_alias(w, alias);
            w.write("#pragma endregion forward_declarations\n\n");

            w.write("#pragma region delegates\n");
            write_delegates(w, members.delegates);
            w.write("#pragma endregion delegates\n\n");
        }
        {
            auto wrap = wrap_impl_namespace(w);

            w.write("#pragma region guids\n");
            w.write_each<write_guid>(members.interfaces);
            w.write("#pragma endregion guids\n\n");
        }

        write_close_file_guard(w);
        w.swap();

        write_preamble(w);
        write_open_file_guard(w, ns, '0');
        w.write("#include \"win32/base.h\"\n");

        for (auto const& entry : w.extern_depends)
        {
            auto guard = wrap_type_namespace(w, entry.first);
            w.write_each<write_extern_forward>(entry.second);
        }
        write_dependency_forwards(w);
        for (auto dependency : w.native_namespace_dependencies) if (dependency != ns) w.write_depends(dependency, '0');

        w.save_header('0');
    }

    static void write_namespace_1_h(std::string_view const& ns, cache::namespace_members const& members)
    {
        writer w;
        w.type_namespace = ns;
        
        w.write("#include \"win32/impl/complex_structs.h\"\n");

        {
            auto wrap = wrap_type_namespace(w, ns);

            w.write("#pragma region interfaces\n");
            //write_interfaces(w, members.interfaces);
            w.write("#pragma endregion interfaces\n\n");
        }

        write_close_file_guard(w);
        w.swap();
        write_preamble(w);
        write_open_file_guard(w, ns, '1');

        for (auto&& depends : w.depends)
        {
            w.write_depends(depends.first, '0');
        }

        w.write_depends(w.type_namespace, '0');
        w.save_header('1');
    }

    static void write_namespace_2_h(std::string_view const& ns, cache::namespace_members const& members)
    {
        writer w;
        w.type_namespace = ns;

        w.write("#include \"win32/impl/complex_interfaces.h\"\n");

        {
            // No namespace
            //w.write("#pragma region abi_methods\n");
            //w.write_each<write_class_abi>(members.classes);
            //w.write("#pragma endregion abi_methods\n\n");
        }

        write_close_file_guard(w);
        w.swap();
        write_preamble(w);
        write_open_file_guard(w, ns, '2');

        w.write_depends(w.type_namespace, '1');
        // Workaround for https://github.com/microsoft/cppwin32/issues/2
        for (auto&& extern_depends : w.extern_depends)
        {
            auto guard = wrap_type_namespace(w, extern_depends.first);
            w.write_each<write_extern_forward>(extern_depends.second);
        }
        w.save_header('2');
    }

    static void write_namespace_h(std::string_view const& ns, cache::namespace_members const& members)
    {
        writer w;
        w.type_namespace = ns;
        {
            auto wrap = wrap_type_namespace(w, ns);

            w.write("#pragma region methods\n");
            w.write_each<write_class>(members.classes);
            w.write("#pragma endregion methods\n\n");
        }

        if (auto found = late_macro_consumers.find(ns); found != late_macro_consumers.end())
            for (auto consumer : found->second)
                if (consumer != ns) w.write_root_include(consumer);
        std::vector<std::pair<std::string_view, std::string>> selections;
        for (auto const& entry : conditional_alias_indices)
            if (entry.first.first == ns && is_late_conditional_alias(ns, entry.first.second)) selections.push_back(entry.first);
        write_conditional_groups(w, selections, false);
        if (auto found = late_record_macros.find(ns); found != late_record_macros.end())
            for (auto const& entry : found->second)
            {
                auto arches = GetSupportedArchitectures(entry.first);
                check_for_write_defined_arches__part_head(w, arches);
                w.write("#define % %\n", entry.second.first, entry.second.second);
                check_for_write_defined_arches__part_tail(w, arches);
            }

        write_close_file_guard(w);
        w.swap();
        write_preamble(w);
        write_open_file_guard(w, ns);
        write_version_assert(w);

        w.write_depends(w.type_namespace, '2');
        for (auto dependency : w.native_namespace_dependencies) if (dependency != ns) w.write_depends(dependency, '0');
        // Workaround for https://github.com/microsoft/cppwin32/issues/2
        for (auto&& extern_depends : w.extern_depends)
        {
            auto guard = wrap_type_namespace(w, extern_depends.first);
            w.write_each<write_extern_forward>(extern_depends.second);
        }
        w.save_header();
    }

	static void write_EnumFromUdtConst_h(cache const& c)
	{
		writer w;

		write_preamble(w);
        write_open_file_guard(w, "EnumFromUdtConst");

		for (auto&& item : mapEnumFromUdtConst)
		{
            auto& type = item.first;
            auto& vec = item.second;
            w.write("enum MACRO_% : % {\n", type.udt, type.base);
            for (auto& str: vec)
            {
                w.write("	%,\n", str);
            }
            w.write("};\n");
		}

        write_close_file_guard(w);

		w.flush_to_file(settings.output_folder + "win32/impl/EnumFromUdtConst.h");
	}


    static void write_complex_structs_h(cache const& c)
    {
        writer w;

        type_dependency_graph graph;
        for (auto&& [ns, members] : c.namespaces())
        {
            for (auto&& s : members.structs)
            {
                graph.add_struct(s);
            }
        }

        graph.walk_graph([&](TypeDef const& type)
            {
                if (!is_nested(type))
                {
                    auto arches = GetSupportedArchitectures(type);
                    check_for_write_defined_arches__part_head(w, arches);
                    {
						if (IsCppTypedef(type))
						{
                            if (!typedef_record_owner(type)) w.WriteCppTypedef(type);
						}
						else
						{
							write_struct(w, type, arches);
						}
                    }
                    check_for_write_defined_arches__part_tail(w, arches);
                    if (!IsCppTypedef(type))
                    {
                        write_owner_conditionals(w, type);
                    }
                }
            });

        write_close_file_guard(w);
        w.swap();

        write_preamble(w);
        write_open_file_guard(w, "complex_structs");
        for (auto dependency : w.native_namespace_dependencies) w.write_depends(dependency, '0');

        for (auto&& depends : w.depends)
        {
            w.write_depends(depends.first, '0');
        }

        w.flush_to_file(settings.output_folder + "win32/impl/complex_structs.h");
    }

    static void write_complex_interfaces_h(cache const& c)
    {
        writer w;

        type_dependency_graph graph;
        for (auto&& [ns, members] : c.namespaces())
        {
            for (auto&& s : members.interfaces)
            {
                graph.add_interface(s);
            }
        }

        graph.walk_graph([&](TypeDef const& type)
            {
                if (!is_nested(type))
                {
                    write_interface(w, type);
                }
            });

        write_close_file_guard(w);
        w.swap();

        write_preamble(w);
        write_open_file_guard(w, "complex_interfaces");
        for (auto dependency : w.native_namespace_dependencies) w.write_depends(dependency, '0');

        for (auto&& depends : w.depends)
        {
            w.write_depends(depends.first, '1');
        }
        // Workaround for https://github.com/microsoft/cppwin32/issues/2
        for (auto&& extern_depends : w.extern_depends)
        {
            auto guard = wrap_type_namespace(w, extern_depends.first);
            w.write_each<write_extern_forward>(extern_depends.second);
        }

        w.flush_to_file(settings.output_folder + "win32/impl/complex_interfaces.h");
    }

    static void write_native_contracts(cache const& metadata, std::string const& path)
    {
        writer ownership;
        auto record_alias = [&](auto const& key, std::string_view alias, std::string_view kind)
        {
            auto owner = metadata.find_required(key.first, key.second);
            auto origin = get_native_record_origin(owner);
            ownership.write("%\t%\t%\t%\t%\n", kind, key.first, key.second, origin.tag.empty() ? get_cpp_type_name(owner) : origin.tag, alias);
        };
        for (auto const& entry : related_typedefs)
            for (auto const& alias : entry.second) record_alias(entry.first, get_cpp_type_name(alias), "metadata");
        for (auto const& entry : related_native_aliases)
            for (auto const& alias : entry.second) record_alias(entry.first, alias, "native");
        ownership.flush_to_file(path + ".aliases.tsv");
        writer output;
        size_t count = 0;
        auto add = [&](auto const& row, Architecture architecture)
        {
            auto attribute = get_attribute(row, "Windows.Win32.Foundation.Metadata", "NativeDeclaratorAttribute");
            if (!attribute) return;
            auto value = attribute.Value();
            auto dependencies = native_declaration_catalog::argument(value, 1);
            size_t begin = 0;
            while (begin < dependencies.size())
            {
                auto end = dependencies.find(';', begin);
                auto name = dependencies.substr(begin, end == std::string::npos ? end : end - begin);
                auto definition = row.get_database().get_cache().find(name);
                if (definition && definition.TypeDisplayName() != "WCHAR") output.add_depends(definition);
                if (end == std::string::npos) break;
                begin = end + 1;
            }
            check_for_write_defined_arches__part_head(output, architecture);
            auto name = std::string("NativeContract_") + std::to_string(count++);
            output.write("typedef %;\n", native_declaration(native_declaration_catalog::argument(value, 0), name));
            check_for_write_defined_arches__part_tail(output, architecture);
        };
        std::set<database const*> databases;
        for (auto const& entry : metadata.namespaces())
            for (auto const& item : entry.second.types) databases.insert(&item.second.get_database());
        for (auto db : databases)
            for (auto const& type : db->TypeDef)
            {
                auto architecture = GetSupportedArchitectures(type);
                auto parent = type;
                while (architecture == Architecture::None && is_nested(parent))
                {
                    parent = parent.EnclosingType();
                    architecture = GetSupportedArchitectures(parent);
                }
                for (auto const& field : type.FieldList()) add(field, architecture);
                for (auto const& method : type.MethodList())
                {
                    auto method_architecture = GetSupportedArchitectures(method);
                    if (method_architecture == Architecture::None) method_architecture = architecture;
                    for (auto const& parameter : method.ParamList()) add(parameter, method_architecture);
                }
            }
        for (auto const& alias : conditional_native_aliases)
        {
            output.write("#if %\n", alias.condition);
            auto scope = alias.scope;
            if (scope.empty() && alias.kind == "macro" && !IsHiddenTypeNamespace(alias.owner)) scope = std::string(alias.owner.TypeNamespace());
            {
                auto guard = wrap_type_namespace(output, scope, scope.empty());
                output.write("using ConditionalAliasContract_% = %;\n", static_cast<uint32_t>(count++), alias.name);
            }
            output.write("#endif\n");
        }
        for (auto const& entry : conditional_alias_indices)
        {
            auto const& alias = conditional_native_aliases[entry.second.front()];
            std::set<std::string> targets;
            for (auto index : entry.second) targets.insert(conditional_native_aliases[index].target);
            if (targets.size() != 1 || !native_declarations.aliases.count(alias.name)) continue;
            output.write("#if !(%)\n", alias.condition);
            output.write("using OriginalAliasContract_% = %;\n", static_cast<uint32_t>(count++), alias.name);
            output.write("#endif\n");
        }
        output.swap();
        output.write("#pragma once\nnamespace PSDK {}\n#include \"win32/base.h\"\n");
        for (auto const& entry : metadata.namespaces()) output.write_root_include(entry.first);
        writer closure;
        closure.depends = output.depends;
        std::set<TypeDef> emitted;
        while (true)
        {
            std::vector<TypeDef> pending;
            for (auto const& entry : closure.depends)
                for (auto const& type : entry.second) if (emitted.insert(type).second) pending.push_back(type);
            if (pending.empty()) break;
            for (auto const& type : pending) write_forward(closure, type);
        }
        for (auto const& type : emitted)
            if (!IsCppTypedef(type) && get_category(type) != category::delegate_type) write_forward(output, type);
        for (auto const& type : emitted)
            if (IsCppTypedef(type)) write_forward(output, type);
        dependency_sorter delegates;
        for (auto const& type : emitted)
            if (get_category(type) == category::delegate_type) delegates.add_delegate(type);
        for (auto const& type : delegates.sort()) write_forward(output, type);
        output.flush_to_file(path);
    }

}
