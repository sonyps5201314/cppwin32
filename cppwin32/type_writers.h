#pragma once

#include <winmd_reader.h>
#include "text_writer.h"
#include "helpers.h"

namespace cppwin32
{
    using namespace winmd::reader;

    template <typename First, typename... Rest>
    auto get_impl_name(First const& first, Rest const&... rest)
    {
        std::string result;
        result.reserve(std::string_view(first).size() + ((1 + std::string_view(rest).size()), ...));
        ((result += first), (((result += '_') += rest), ...));
        std::transform(result.begin(), result.end(), result.begin(), [](char c)
            {
                return c == '.' ? '_' : c;
            });
        return result;
    }

    std::set<std::pair<std::string_view, std::string_view>> scoped_native_types;
    void check_native_name_conflicts(TypeDef const& type)
    {
        auto origin = get_native_record_origin(type);
        if (!origin.present) return;
        auto check = [&](std::string_view name, bool primary)
        {
            if (name.empty() || name == type.TypeDisplayName()) return;
            for (auto const& entry : type.get_cache().namespaces())
            {
                auto found = entry.second.types.find(name);
                if (found == entry.second.types.end() || found->second == type || record_typedef_target(found->second) == type) continue;
                if (primary)
                    scoped_native_types.emplace(type.TypeNamespace(), type.TypeDisplayName());
            }
        };
        check(origin.tag, true);
        auto primary = get_cpp_type_name(type);
        for (auto name : origin.typedefs) check(name, name == primary);
        for (auto const& macro : origin.macros) check(macro.first, false);
    }

	std::map<std::string_view, std::string_view> mapNamespaceHiddenType;
	void CheckForHideTypeNamespace(TypeDef const& type, const std::string_view& ns)
	{
#if HIDE_TYPE_NAMESPACE
        std::string_view type_name = type.TypeDisplayName();
		bool not_found = mapNamespaceHiddenType.find(type_name) == mapNamespaceHiddenType.end();
		if (not_found)
		{
            mapNamespaceHiddenType[type_name] = ns;
		}
#endif
	}
    
    template<class TypeDef_Or_TypeRef>
	bool IsHiddenTypeNamespace(TypeDef_Or_TypeRef const& type)
	{
#if HIDE_TYPE_NAMESPACE
		std::string_view type_name = type.TypeDisplayName();
		auto v = mapNamespaceHiddenType.find(type_name);
		bool result = v != mapNamespaceHiddenType.end() && v->second == type.TypeNamespace();
		return result && scoped_native_types.count({type.TypeNamespace(), type_name}) == 0;
#else
        return false;
#endif
	}

    std::map<std::string_view, std::vector<TypeDef>> native_typedef_owners;

    void initialize_native_typedef_owners(cache const& metadata)
    {
        for (auto const& entry : metadata.namespaces())
            for (auto const& item : entry.second.types)
                if (!is_conditional_alias(item.second)) native_typedef_owners[get_cpp_type_name(item.second)].push_back(item.second);
    }

    bool can_write_native_typedef(TypeDef const& type, std::string_view name)
    {
        if (is_conditional_record_alias(type, name)) return false;
        if (is_nested(type) || name == get_cpp_type_name(type)) return true;
        auto owners = native_typedef_owners.find(name);
        if (owners == native_typedef_owners.end()) return true;
        auto scope = IsHiddenTypeNamespace(type) ? std::string_view{} : type.TypeNamespace();
        for (auto const& owner : owners->second)
        {
            auto owner_scope = IsHiddenTypeNamespace(owner) ? std::string_view{} : owner.TypeNamespace();
            if (scope != owner_scope) continue;
            if ((type.TypeNamespace() == owner.TypeNamespace() && type.TypeDisplayName() == owner.TypeDisplayName()) || record_typedef_target(owner) == type) continue;
            return false;
        }
        return true;
    }

    TypeDef resolve_native_record_reference(TypeDef const& target, std::string_view name, Architecture arches)
    {
        if (!target || can_write_native_typedef(target, name)) return target;
        auto owners = native_typedef_owners.find(name);
        if (owners == native_typedef_owners.end()) return target;
        auto scope = IsHiddenTypeNamespace(target) ? std::string_view{} : target.TypeNamespace();
        for (auto const& owner : owners->second)
        {
            auto owner_scope = IsHiddenTypeNamespace(owner) ? std::string_view{} : owner.TypeNamespace();
            if (scope != owner_scope) continue;
            for (auto variant = owner; variant; variant = variant.next ? *variant.next : TypeDef{})
            {
                auto supported = GetSupportedArchitectures(variant);
                if (arches == Architecture::None || supported == Architecture::None || (supported & arches)) return variant;
            }
        }
        return target;
    }

    TypeDef native_record_reference_target(Field const& field, TypeDef const& target, Architecture arches)
    {
        auto attribute = get_attribute(field, "Windows.Win32.Foundation.Metadata", "NativeRecordReferenceAttribute");
        if (!attribute) return target;
        auto signature = attribute.Value();
        auto name = std::get<std::string_view>(std::get<ElemSig>(signature.FixedArgs()[0].value).value);
        return resolve_native_record_reference(target, name, arches);
    }

    struct writer;
    struct finish_with
    {
        writer& w;
        void (*finisher)(writer&);

        finish_with(writer& w, void (*finisher)(writer&)) : w(w), finisher(finisher) {}
        finish_with(finish_with const&) = delete;
        void operator=(finish_with const&) = delete;

        ~finish_with() { finisher(w); }
    };
    [[nodiscard]] finish_with wrap_type_namespace(writer& w, std::string_view const& ns, bool cancel = false);
    void write_forward(writer& w, TypeDef const& type);

    struct writer : writer_base<writer>
    {
        using writer_base<writer>::write;

        struct depends_compare
        {
            bool operator()(TypeDef const& left, TypeDef const& right) const
            {
                return left.TypeDisplayName() < right.TypeDisplayName();
            }
            bool operator()(TypeRef const& left, TypeRef const& right) const
            {
                return left.TypeDisplayName() < right.TypeDisplayName();
            }
        };

        std::string type_namespace;
        bool abi_types{};
        bool full_namespace{};
        bool consume_types{};
        std::string_view native_primitive_type;
        std::set<std::string_view> native_namespace_dependencies;
        std::set<TypeDef> forwarding;
        std::set<std::pair<TypeDef, Architecture>> forwarded_metadata_types;
        std::set<std::pair<TypeDef, Architecture>> declared_metadata_types;
        std::set<std::pair<std::string, Architecture>> native_aliases_written;
        std::set<std::string> native_aliases_pending;
        std::set<TypeDef> related_typedefs_pending;
        std::set<std::pair<TypeDef, Architecture>> related_typedefs_written;
        std::set<std::tuple<std::string_view, std::string, Architecture>> conditional_aliases_written;
        std::vector<Architecture> architecture_stack;
        Architecture current_architecture = Architecture::All;
        std::map<std::string_view, std::set<TypeDef, depends_compare>> depends;
        std::map<std::string_view, std::set<TypeDef, depends_compare>>* pdepends_In_WriteCppTypedef = nullptr;
        std::map<std::string_view, std::set<TypeRef, depends_compare>> extern_depends;

        bool metadata_type_written(std::set<std::pair<TypeDef, Architecture>> const& declarations, TypeDef const& type, Architecture arches) const
        {
            auto written = Architecture::None;
            for (auto entry = declarations.lower_bound({type, Architecture::None}); entry != declarations.end() && entry->first == type; ++entry)
                written = static_cast<Architecture>(written | entry->second);
            return arches != Architecture::None && (written & arches) == arches;
        }

        template<typename T>
        struct member_value_guard
        {
            writer* const owner;
            T writer::* const member;
            T const previous;
            explicit member_value_guard(writer* arg, T writer::* ptr, T value) :
                owner(arg), member(ptr), previous(std::exchange(owner->*member, value))
            {
            }

            ~member_value_guard()
            {
                owner->*member = previous;
            }

            member_value_guard(member_value_guard const&) = delete;
            member_value_guard& operator=(member_value_guard const&) = delete;
        };

        [[nodiscard]] auto push_abi_types(bool value)
        {
            return member_value_guard(this, &writer::abi_types, value);
        }

        [[nodiscard]] auto push_full_namespace(bool value)
        {
            return member_value_guard(this, &writer::full_namespace, value);
        }

        [[nodiscard]] auto push_consume_types(bool value)
        {
            return member_value_guard(this, &writer::consume_types, value);
        }

        void write_root_include(std::string_view const& include)
        {
            auto format = R"(#include %win32/%.h%
)";

            write(format,
                settings.brackets ? '<' : '\"',
                include,
                settings.brackets ? '>' : '\"');
        }

        void add_depends(TypeDef const& type)
        {
            auto ns = type.TypeNamespace();
            if (pdepends_In_WriteCppTypedef)
            {
                (*pdepends_In_WriteCppTypedef)[ns].insert(type);
                depends[ns].erase(type);
            }
            else if (ns != type_namespace)
            {
                depends[ns].insert(type);
            }
        }

        void add_extern_depends(TypeRef const& type)
        {
            auto ns = type.TypeNamespace();
            //XLANG_ASSERT(ns != type_namespace);
            if (ns != type_namespace)
            {
                extern_depends[ns].insert(type);
            }
        }

        void write_depends(std::string_view const& ns, char impl = 0)
        {
            if (impl)
            {
                write_root_include(write_temp("impl/%.%", ns, impl));
            }
            else
            {
                write_root_include(ns);
            }
        }

        template <typename T>
        void write_value(T value)
        {
            char buffer[128];
            char* first = buffer;
            char* last = std::end(buffer);

            if constexpr (std::numeric_limits<T>::is_integer)
            {
                int base = 10;
                if constexpr (!std::numeric_limits<T>::is_signed)
                {
                    *first++ = '0';
                    *first++ = 'x';
                    base = 16;
                }
                auto end = std::to_chars(first, last, value, base).ptr;
                write(std::string_view{ buffer, static_cast<size_t>(end - buffer) });
            }
            else
            {
                static_assert(std::is_same_v<float, T> || std::is_same_v<double, T>);
                *first++ = '0';
                *first++ = 'x';
                auto end = std::to_chars(first, last, value, std::chars_format::hex).ptr;
                // Put the leading '-' in front of '0x'
                if (*first == '-')
                {
                    std::rotate(buffer, first, first + 1);
                }
                write(std::string_view{ buffer, static_cast<size_t>(end - buffer) });
            }
        }

        void write_code(std::string_view const& value)
        {
            for (auto&& c : value)
            {
                if (c == '.')
                {
                    write("::");
                }
                else
                {
                    write(c);
                }
            }
        }

        void write(ConstantType type)
        {
            switch (type)
            {
			case ConstantType::Boolean:
				write("bool");
				break;
            case ConstantType::UInt8:
                write("unsigned char");
                break;
            case ConstantType::Int8:
                write("signed char");
                break;
            case ConstantType::UInt16:
                write("unsigned short");
                break;
            case ConstantType::Int16:
                write("short");
                break;
            case ConstantType::UInt32:
                write("unsigned int");
                break;
            case ConstantType::Int32:
                write("int");
                break;
            case ConstantType::UInt64:
                write("unsigned long long");
                break;
            case ConstantType::Int64:
                write("long long");
                break;
            case ConstantType::Float32:
                write("float");
                break;
            case ConstantType::Float64:
                write("double");
                break;
            case ConstantType::String:
                write("wchar_t const*");
                break;
            default:
                throw_invalid("Invalid ConstantType");
                break;
            }
        }

        void write(Constant const& value)
        {
            switch (value.Type())
            {
			case ConstantType::Boolean:
				write(value.ValueBoolean() ? "true" : "false");
				break;
            case ConstantType::UInt8:
                write_value(value.ValueUInt8());
                break;
            case ConstantType::Int8:
                write_value(value.ValueInt8());
                break;
            case ConstantType::UInt16:
                write_value(value.ValueUInt16());
                break;
            case ConstantType::Int16:
                write_value(value.ValueInt16());
                break;
            case ConstantType::Int32:
                write_value(value.ValueInt32());
                break;
            case ConstantType::UInt32:
                write_value(value.ValueUInt32());
                break;
            case ConstantType::Int64:
                write_value(value.ValueInt64());
                break;
            case ConstantType::UInt64:
                write_value(value.ValueUInt64());
                break;
            case ConstantType::Float32:
                write_value(value.ValueFloat32());
                break;
            case ConstantType::Float64:
                write_value(value.ValueFloat64());
                break;
            case ConstantType::String:
                write(R"d(LR"(%)")d", value.ValueString());
                break;
            default:
                throw std::invalid_argument("Unexpected constant type");
            }
        }

        void write(std::u16string_view const& str)
        {
            auto const data = reinterpret_cast<wchar_t const*>(str.data());
            auto const size = ::WideCharToMultiByte(CP_UTF8, 0, data, static_cast<int32_t>(str.size()), nullptr, 0, nullptr, nullptr);
            if (size == 0)
            {
                return;
            }
            std::string result(size, '?');
            ::WideCharToMultiByte(CP_UTF8, 0, data, static_cast<int32_t>(str.size()), result.data(), size, nullptr, nullptr);
            write(result);
        }

        void write(TypeDef const& type)
        {
            if (auto alias = find_conditional_alias(type.TypeNamespace(), type.TypeDisplayName()); alias && is_conditional_alias(type))
            {
                add_depends(alias->owner);
                native_namespace_dependencies.insert(alias->owner.TypeNamespace());
                if (!alias->scope.empty()) write("win32::@::", alias->scope);
                write(alias->name);
                return;
            }
            if (auto nested = nested_typedef_target(type))
            {
                write(nested.EnclosingType());
                write("::%", get_cpp_type_name(nested));
                return;
            }
            add_depends(type);
            if (is_nested(type))
            {
                write(get_cpp_type_name(type));
            }
            else
            {
                if (IsHiddenTypeNamespace(type))
                {
                    write("%", get_cpp_type_name(type));
                }
                else
                {
					if (full_namespace)
					{
						write("win32::");
					}
					write("@::%", type.TypeNamespace(), get_cpp_type_name(type));
                }
            }
        }

        void write(TypeRef const& type)
        {
            if (type.TypeNamespace() == "System" && type.TypeDisplayName() == "Guid")
            {
                write("GUID");
            }
            else if (is_nested(type))
            {
                write(type.TypeDisplayName());
            }
            else
            {
                Architecture arches = GetSupportedArchitectures(type);
                auto type_def = find(type, arches);
                if (type_def)
                {
                    write(type_def);
                    return;
                }
                add_extern_depends(type);
                if (IsHiddenTypeNamespace(type))
                {
                    write("%", type.TypeDisplayName());
                }
                else
                {
					if (full_namespace)
					{
						write("win32::");
					}
					write("@::%", type.TypeNamespace(), type.TypeDisplayName());
                }
            }
        }

        void write(coded_index<TypeDefOrRef> const& type)
        {
            switch (type.type())
            {
            case TypeDefOrRef::TypeDef:
                write(type.TypeDef());
                break;
            case TypeDefOrRef::TypeRef:
                write(type.TypeRef());
                break;
            default:
                throw std::invalid_argument("Unexpected TypeDefOrRef type");
            }
        }

        void write(TypeSig const& signature)
        {
            call(signature.Type(),
                [&](ElementType type)
                {
                    if (!native_primitive_type.empty())
                    {
                        write(native_primitive_type);
                    }
                    else switch (type)
                    {
                    case ElementType::Boolean:
                        write("bool");
                        break;
                    case ElementType::Char:
                        write("WCHAR");
                        break;
                    case ElementType::I1:
                        write("signed char");
                        break;
                    case ElementType::U1:
                        write("unsigned char");
                        break;
                    case ElementType::I2:
                        write("short");
                        break;
                    case ElementType::U2:
                        write("unsigned short");
                        break;
                    case ElementType::I4:
                        write("int");
                        break;
                    case ElementType::U4:
                        write("unsigned int");
                        break;
                    case ElementType::I8:
                        write("long long");
                        break;
                    case ElementType::U8:
                        write("unsigned long long");
                        break;
                    case ElementType::R4:
                        write("float");
                        break;
                    case ElementType::R8:
                        write("double");
                        break;
                    case ElementType::U:
                        write("size_t");
                        break;
                    case ElementType::I:
                        write("intptr_t");
                        break;
                    case ElementType::Void:
                        write("void");
                        break;
                    default:
                        throw std::invalid_argument("Invalid TypeSig type");
                    }
                    for (int i = 0; i < signature.ptr_count(); ++i)
                    {
                        write('*');
                    }
                },
                [&](coded_index<TypeDefOrRef> const& type)
                {
                    TypeDef type_def;
                    category cat = (category)-1;
                    auto type_type = type.type();
                    if (type_type == TypeDefOrRef::TypeRef)
                    {
                        const auto& type_ref = type.TypeRef();
                        Architecture arches = GetSupportedArchitectures(type_ref);
                        type_def = find(type, arches);
                        //全局类多重指针必须前面加struct/union等类型关键字才行
                        if (signature.element_type() == ElementType::Class && signature.ptr_count() > 0)
                        {
                            if (IsHiddenTypeNamespace(type_ref))
                            {
                                std::string_view type_keyword;
                                if (type_def)
                                {
                                    cat = get_category(type_def);
                                    if (cat == category::struct_type)
                                    {
                                        type_keyword = is_union(type_def) ? "union" : "struct";
                                    }
                                    else if (cat == category::interface_type || cat == category::class_type)
                                    {
                                        type_keyword = "struct";
                                    }
                                }
                                else
                                {
                                    assert(false);
                                }
                                if (!type_keyword.empty())
                                {
                                    write(type_keyword);
                                    write(' ');
                                }
                            }
                        }
                    }
                    else if(type_type == TypeDefOrRef::TypeDef)
                    {
                        type_def = type.TypeDef();
                    }
                    write(type);
                    for (int i = 0; i < signature.ptr_count(); ++i)
                    {
                        write('*');
                    }
                    if (signature.element_type() == ElementType::Class || (type_def && is_interface_typedef(type_def, GetSupportedArchitectures(type_def))))
                    {
                        if (cat == (category)-1)
                        {
                            if (type_def)
                            {
                                cat = get_category(type_def);
                            }
                        }
                        if (cat != category::delegate_type)
                        {
                            write('*');
                        }
                    }
                },
                [&](auto&& type)
                {
                    throw std::invalid_argument("Invalid TypeSig type");
                });
        }

        void write(RetTypeSig const& value)
        {
            if (value)
            {
                write(value.Type());
            }
            else
            {
                write("void");
            }
        }

        void save_header(char impl = 0)
        {
            auto filename{ settings.output_folder + "win32/" };
            if (impl)
            {
                filename += "impl/";
            }

            filename += type_namespace;

            if (impl)
            {
                filename += '.';
                filename += impl;
            }

            filename += ".h";
            flush_to_file(filename);
        }

		struct tagForceTypeDef
		{
			const char* name;
			const char* type;
		};
		static constexpr tagForceTypeDef ForceTypeDefs[] = { {"PSTR","char*"},{"PWSTR","wchar_t*"} };
        static constexpr tagForceTypeDef ForceTypeDefs_Const[] = { {"PCSTR","const char*"},{"PCWSTR","const wchar_t*"} };
		void WriteCppTypedef(TypeDef const& type, bool definition = false)
		{
            if (is_conditional_alias(type) || nested_typedef_target(type) || is_record_tag_typedef(type)) return;
            if (!definition && metadata_type_written(declared_metadata_types, type, current_architecture)) return;
            if (type.TypeDisplayName() == "va_list")
            {
                auto guard = wrap_type_namespace(*this, type.TypeNamespace(), IsHiddenTypeNamespace(type));
                write("    typedef ::va_list va_list;\n");
                if (current_architecture != Architecture::None) declared_metadata_types.emplace(type, current_architecture);
                return;
            }
			int i = 0;

			for (auto&& field : type.FieldList())
			{
				if (i > 0)
				{
					assert(false);
					break;
				}

                bool outputed = false;
                auto type_name = type.TypeDisplayName();
                {
					int i = 0;
					for (auto ftd : ForceTypeDefs)
					{
						if (type_name == ftd.name)
						{
                            auto guard = wrap_type_namespace(*this, type.TypeNamespace(), IsHiddenTypeNamespace(type));

							auto const signature = field.Signature();
							auto const field_type = ftd.type;
							write("    typedef % %;\n",
								field_type,
                                type_name);
							write("    typedef % %;\n",
                                ForceTypeDefs_Const[i].type,
                                ForceTypeDefs_Const[i].name);

                            outputed = true;
							break;
						}
						i++;
					}
                }

				if (!outputed)
				{
					auto const signature = field.Signature();
					auto const field_type = signature.Type();

                    std::map<std::string_view, std::set<TypeDef, depends_compare>>* Last_pdepends_In_WriteCppTypedef = pdepends_In_WriteCppTypedef;
                    std::map<std::string_view, std::set<TypeDef, depends_compare>> depends;
                    pdepends_In_WriteCppTypedef = &depends;

                    std::string array_suffix;
                    if (field_type.is_array())
                    {
                        for (auto size : field_type.array_sizes())
                        {
                            array_suffix += write_temp("[%]", size);
                        }
                    }
                    if (type_name == "WCHAR")
                    {
                        write("    typedef wchar_t WCHAR;\n");
                        if (current_architecture != Architecture::None) declared_metadata_types.emplace(type, current_architecture);
                        pdepends_In_WriteCppTypedef = Last_pdepends_In_WriteCppTypedef;
                        return;
                    }
                    auto str_typedef = is_interface_typedef(type, GetSupportedArchitectures(type))
                        ? write_temp("    typedef % %;\n", std::get<coded_index<TypeDefOrRef>>(field_type.Type()), type_name)
                        : write_temp("    typedef % %%;\n", field_type, type_name, array_suffix);
                    
                    pdepends_In_WriteCppTypedef = Last_pdepends_In_WriteCppTypedef;

                    for (auto const& [ns, definitions] : depends)
                    {
                        for (auto const& definition : definitions)
                        {
                            auto const& namespaces = definition.get_cache().namespaces();
                            auto entry = namespaces.find(ns);
                            if (entry == namespaces.end())
                            {
                                write_forward(*this, definition);
                                continue;
                            }

                            auto const& members = entry->second;
                            auto variants = members.types_same_name.find(definition.TypeDisplayName());
                            if (variants == members.types_same_name.end())
                            {
                                write_forward(*this, definition);
                            }
                            else
                            {
                                write_each<write_forward>(variants->second);
                            }
                        }
                    }

                    {
						auto guard = wrap_type_namespace(*this, type.TypeNamespace(), IsHiddenTypeNamespace(type));

                        write(str_typedef);
                        if (current_architecture != Architecture::None) declared_metadata_types.emplace(type, current_architecture);
                    }
				}

				i++;
			}
            if (current_architecture != Architecture::None) declared_metadata_types.emplace(type, current_architecture);
		}
    };

	static void check_for_write_defined_arches__part_head(writer& w, Architecture arches)
	{
		if (arches != Architecture::None)
		{
            w.architecture_stack.push_back(w.current_architecture);
            w.current_architecture = static_cast<Architecture>(w.current_architecture & arches);
			bool has_least_one = false;
			std::string str;
			str = "#if ";
			if (arches & Architecture::X86)
			{
				str += "defined(_M_IX86)";
				has_least_one = true;
			}
			if (arches & Architecture::X64)
			{
				if (has_least_one)
				{
					str += " || ";
				}
				str += "defined(_M_AMD64)";
				has_least_one = true;
			}
			if (arches & Architecture::Arm64)
			{
				if (has_least_one)
				{
					str += " || ";
				}
				str += "defined(_M_ARM64)";
				has_least_one = true;
			}
			str += "\n";
			w.write(str);
		}
	}
	static void check_for_write_defined_arches__part_tail(writer& w, Architecture arches)
	{
		if (arches != Architecture::None)
		{
			auto format = R"(#endif
)";
			w.write(format);
            w.current_architecture = w.architecture_stack.back();
            w.architecture_stack.pop_back();
		}
	}
}