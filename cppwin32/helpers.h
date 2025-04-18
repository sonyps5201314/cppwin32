#pragma once

namespace cppwin32
{
	inline bool starts_with(const std::string_view& value, const char* prefix, size_t prefix_len)
	{
		// Cannot start with an empty string.
		if (prefix_len == 0)
			return false;

		return (value.size() >= prefix_len) &&
			strncmp(value.data(), prefix, prefix_len) == 0;
	}

	inline bool ends_with(const std::string_view& value, const char* suffix, size_t suffix_len)
	{
		return (value.size() >= suffix_len) &&
			strcmp(value.data() + value.size() - suffix_len, suffix) == 0;
	}

	template<size_t LEN>
	bool starts_with(const std::string_view& value, const char(&prefix)[LEN])
	{
		return starts_with(value, prefix, LEN - 1);
	}

	template<size_t LEN>
	bool ends_with(const std::string_view& str, const char(&suffix)[LEN])
	{
		return ends_with(str, suffix, LEN - 1);
	}


    using namespace winmd::reader;

    template <typename...T> struct visit_overload : T... { using T::operator()...; };

    template <typename V, typename...C>
    auto call(V&& variant, C&&...call)
    {
        return std::visit(visit_overload<C...>{ std::forward<C>(call)... }, std::forward<V>(variant));
    }

    struct type_name
    {
        std::string_view name;
        std::string_view name_space;

        explicit type_name(TypeDef const& type) :
            name(type.TypeDisplayName()),
            name_space(type.TypeNamespace())
        {
        }

        explicit type_name(TypeRef const& type) :
            name(type.TypeDisplayName()),
            name_space(type.TypeNamespace())
        {
        }

        explicit type_name(coded_index<TypeDefOrRef> const& type)
        {
            auto const& [type_namespace, type_name] = get_type_namespace_and_name(type);
            name_space = type_namespace;
            name = type_name;
        }
    };

    bool operator==(type_name const& left, type_name const& right)
    {
        return left.name == right.name && left.name_space == right.name_space;
    }

    bool operator==(type_name const& left, std::string_view const& right)
    {
        if (left.name.size() + 1 + left.name_space.size() != right.size())
        {
            return false;
        }

        if (right[left.name_space.size()] != '.')
        {
            return false;
        }

        if (0 != right.compare(left.name_space.size() + 1, left.name.size(), left.name))
        {
            return false;
        }

        return 0 == right.compare(0, left.name_space.size(), left.name_space);
    }

    struct method_signature
    {
        explicit method_signature(MethodDef const& method) :
            m_method(method),
            m_signature(method.Signature())
        {
            auto params = method.ParamList();

            if (m_signature.ReturnType() && params.first != params.second && params.first.Sequence() == 0)
            {
                m_return = params.first;
                ++params.first;
            }

            for (uint32_t i{}; i != size(m_signature.Params()); ++i)
            {
                m_params.emplace_back(params.first + i, &m_signature.Params().first[i]);
            }
        }

        std::vector<std::pair<Param, ParamSig const*>>& params()
        {
            return m_params;
        }

        std::vector<std::pair<Param, ParamSig const*>> const& params() const
        {
            return m_params;
        }

        auto const& return_signature() const
        {
            return m_signature.ReturnType();
        }

        auto return_param_name() const
        {
            std::string_view name;

            if (m_return && !m_return.Name().empty())
            {
                name = m_return.Name();
            }
            else
            {
                name = "win32_impl_result";
            }

            return name;
        }

        auto return_param() const
        {
            return m_return;
        }

        MethodDef const& method() const
        {
            return m_method;
        }

    private:

        MethodDef m_method;
        MethodDefSig m_signature;
        std::vector<std::pair<Param, ParamSig const*>> m_params;
        Param m_return;
    };

    enum class param_category
    {
        enum_type,
        struct_type,
        array_type,
        fundamental_type,
        interface_type,
        delegate_type,
        generic_type
    };

    inline param_category get_category(coded_index<TypeDefOrRef> const& type, TypeDef* signature_type = nullptr)
    {
        TypeDef type_def;
        if (type.type() == TypeDefOrRef::TypeDef)
        {
            type_def = type.TypeDef();
        }
        else
        {
            auto type_ref = type.TypeRef();
            if (type_name(type_ref) == "System.Guid")
            {
                return param_category::struct_type;
            }
            type_def = find_required(type_ref);
        }

        if (signature_type)
        {
            *signature_type = type_def;
        }

        switch (get_category(type_def))
        {
        case category::interface_type:
            return param_category::interface_type;
        case category::enum_type:
            return param_category::enum_type;
        case category::struct_type:
            return param_category::struct_type;
        case category::delegate_type:
            return param_category::delegate_type;
        default:
            return param_category::generic_type;
        }
    }

    inline param_category get_category(TypeSig const& signature, TypeDef* signature_type = nullptr)
    {
        if (signature.is_szarray())
        {
            return param_category::array_type;
        }

        if (signature.element_type() == ElementType::Class)
        {
            return param_category::interface_type;
        }

        param_category result{};

        call(signature.Type(),
            [&](ElementType type)
            {
                result = param_category::fundamental_type;
            },
            [&](coded_index<TypeDefOrRef> const& type)
            {
                result = get_category(type, signature_type);
            },
                [&](auto&&)
            {
                result = param_category::generic_type;
            });
        return result;
    }

    inline bool is_com_interface(TypeDef const& type)
    {
        if (type.TypeDisplayName() == "IUnknown")
        {
            return true;
        }

        Architecture arches = GetSupportedArchitectures(type);
        for (auto&& base : type.InterfaceImpl())
        {
            auto base_type = find(base.Interface(), arches);
            if (base_type && is_com_interface(base_type))
            {
                return true;
            }
        }
        return false;
    }

    inline bool is_union(TypeDef const& type)
    {
        return type.Flags().Layout() == TypeLayout::ExplicitLayout;
    }

    inline bool is_nested(coded_index<TypeDefOrRef> const& type)
    {
        if (type.type() == TypeDefOrRef::TypeDef)
        {
            return is_nested(type.TypeDef());
        }
        else
        {
            XLANG_ASSERT(type.type() == TypeDefOrRef::TypeRef);
            return is_nested(type.TypeRef());
        }
    }

    MethodDef get_delegate_method(TypeDef const& type)
    {
        MethodDef invoke;
        for (auto&& method : type.MethodList())
        {
            if (method.Name() == "Invoke")
            {
                invoke = method;
                break;
            }
        }
        return invoke;
    }

    coded_index<TypeDefOrRef> get_base_interface(TypeDef const& type)
    {
        auto bases = type.InterfaceImpl();
        if (!empty(bases))
        {
            XLANG_ASSERT(size(bases) == 1);
            return bases.first.Interface();
        }
        return {};
    }

	struct guid
	{
		uint32_t Data1;
		uint16_t Data2;
		uint16_t Data3;
		uint8_t  Data4[8];
	};

	guid to_guid(std::string_view const& str)
	{
		if (str.size() < 36)
		{
			throw_invalid("Invalid GuidAttribute blob");
		}
		guid result;
		auto const data = str.data();
		std::from_chars(data, data + 8, result.Data1, 16);
		std::from_chars(data + 9, data + 13, result.Data2, 16);
		std::from_chars(data + 14, data + 18, result.Data3, 16);
		std::from_chars(data + 19, data + 21, result.Data4[0], 16);
		std::from_chars(data + 21, data + 23, result.Data4[1], 16);
		std::from_chars(data + 24, data + 26, result.Data4[2], 16);
		std::from_chars(data + 26, data + 28, result.Data4[3], 16);
		std::from_chars(data + 28, data + 30, result.Data4[4], 16);
		std::from_chars(data + 30, data + 32, result.Data4[5], 16);
		std::from_chars(data + 32, data + 34, result.Data4[6], 16);
		std::from_chars(data + 34, data + 36, result.Data4[7], 16);
		return result;
	}

	template <typename T>
	auto get_attribute_guid(T const& row)
    {
        std::string result;
		auto attribute = get_attribute(row, "Windows.Win32.Foundation.Metadata", "GuidAttribute");
		if (!attribute)
		{
			return result;
		}

		auto const sig = attribute.Value();
		const auto& args = sig.FixedArgs();
		if (args.size() != 11)
		{
            assert(false);
			return result;
		}

		guid g = {
			std::get <uint32_t>(std::get<ElemSig>(args[0].value).value),
			std::get <uint16_t>(std::get<ElemSig>(args[1].value).value),
			std::get <uint16_t>(std::get<ElemSig>(args[2].value).value),
			{
				std::get <uint8_t>(std::get<ElemSig>(args[3].value).value),
				std::get <uint8_t>(std::get<ElemSig>(args[4].value).value),
				std::get <uint8_t>(std::get<ElemSig>(args[5].value).value),
				std::get <uint8_t>(std::get<ElemSig>(args[6].value).value),
				std::get <uint8_t>(std::get<ElemSig>(args[7].value).value),
				std::get <uint8_t>(std::get<ElemSig>(args[8].value).value),
				std::get <uint8_t>(std::get<ElemSig>(args[9].value).value),
				std::get <uint8_t>(std::get<ElemSig>(args[10].value).value),
			}
		};

		char buffer[128];
        size_t const size = sprintf_s(buffer, "0x%08X,0x%04X,0x%04X,{ 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X }",
            g.Data1,
            g.Data2,
            g.Data3,
            g.Data4[0],
            g.Data4[1],
            g.Data4[2],
            g.Data4[3],
            g.Data4[4],
            g.Data4[5],
            g.Data4[6],
            g.Data4[7]);
        result = buffer;
        return result;
    }

	template <typename T>
    bool IsCppTypedef(T const& type)
    {
        return get_attribute(type, "Windows.Win32.Foundation.Metadata", "NativeTypedefAttribute")
            || get_attribute(type, "Windows.Win32.Foundation.Metadata", "MetadataTypedefAttribute");
    }
}