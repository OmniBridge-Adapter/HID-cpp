#pragma once

#include "tag.hpp"
#include <type_traits>

#include "capped_array.hpp"
#include "hid/usage.hpp"
#include "hid/report_descriptor/parser.hpp"
#include "hid/report_descriptor/helpers.hpp"
#include <span>
#include <tuple>

namespace
{
    template<class TVal>
    struct value_size_raw_type
    {
        using type = std::remove_cvref_t<TVal>;
    };

    template<class TVal>
    requires(std::is_enum_v<std::remove_cvref_t<TVal>>)
    struct value_size_raw_type<TVal>
    {
        using type = std::underlying_type_t<std::remove_cvref_t<TVal>>;
    };

    template<typename TVal>
    constexpr std::size_t ValueSize(TVal Value)
    {
        using raw_t = typename value_size_raw_type<TVal>::type;

        if constexpr (std::is_signed_v<raw_t>)
        {
            const auto signedValue = static_cast<std::int64_t>(Value);
            if (signedValue >= -128 && signedValue <= 127) return 1;
            if (signedValue >= -32768 && signedValue <= 32767) return 2;
            return 4;
        }
        else
        {
            const auto unsignedValue = static_cast<std::uint64_t>(Value);
            if (unsignedValue <= 0xFFULL) return 1;
            if (unsignedValue <= 0xFFFFULL) return 2;
            return 4;
        }
    }

    // template<typename TVal>
    // constexpr std::size_t ValueSize()
    // {
    //     if constexpr (OB::HID::ReportDescriptor::Helpers::is_constant<TVal>::value)
    //     {
    //         return ValueSize<TVal::type>(TVal::value);
    //     }
    //     return 0;
    // }
    template<auto Value>
    constexpr std::size_t ValueSize()
    {
        // if constexpr (OB::HID::ReportDescriptor::Helpers::is_constant<TVal>::value)
        // {
        //     return ValueSize<TVal::type>(TVal::value);
        // }
        return ValueSize(Value);
        // return 0;
    }

}

namespace OB::HID::ReportDescriptor
{
#pragma region Forward Definitions.
    enum class CollectionType;
#pragma endregion

    template<class TTag> 
    // requires (std::is_base_of<TagBase, TTag>::value )
    constexpr ItemType TagType() { return ItemType::Reserved; }

    template<>
    constexpr ItemType TagType<TagMain>() { return ItemType::Main; }
    template<>
    constexpr ItemType TagType<TagLocal>() { return ItemType::Local; }
    template<>
    constexpr ItemType TagType<TagGlobal>() { return ItemType::Global; }

    template<std::size_t SIZE> requires (((SIZE <= 4) && (SIZE != 3)) || (SIZE == std::dynamic_extent))
    class ShortItem
    {
    public:

        template <class TTag>
        constexpr ShortItem(TTag tag)
        {
            std::uint8_t tag_byte = (static_cast<std::uint8_t>(tag)<<4) |
                                (static_cast<std::uint8_t>(TagType<TTag>()) << 2) |
                                static_cast<std::uint8_t>((SIZE == 4U) ? 3U : SIZE);
            m_descriptorArray.push_back(tag_byte);
        }

        template <class TTag, class TData> requires(SIZE > 0)
        constexpr ShortItem(TTag tag, TData val)
        {
            auto value = static_cast<std::uint32_t>(val);
            auto size = std::min(SIZE, ValueSize(value));

            std::uint8_t tag_byte = (static_cast<std::uint8_t>(tag)<<4) |
                                (static_cast<std::uint8_t>(TagType<TTag>()) << 2) |
                                static_cast<std::uint8_t>((size == 4U) ? 3U : size);
            m_descriptorArray.push_back(tag_byte);
            for (std::size_t i = 0; i < size; ++i)
            {
                m_descriptorArray.push_back(static_cast<std::uint8_t>(value));
                value >>= 8;
            }
        }
        constexpr CappedArray<std::uint8_t, SIZE+1> descriptor(DescriptorGlobalState&&) const
        {
            return m_descriptorArray;
        }

        constexpr       CappedArray<std::uint8_t, SIZE+1> &asArray()       { return m_descriptorArray; }
        constexpr const CappedArray<std::uint8_t, SIZE+1> &asArray() const { return m_descriptorArray; }
        constexpr static std::size_t descriptor_max_len = SIZE+1;
    protected:
        CappedArray<std::uint8_t, SIZE+1> m_descriptorArray;
    private:
    };

    template<>
    class ShortItem<std::dynamic_extent>
    {
    public:

        template <class TTag>
        constexpr ShortItem(TTag tag)
        {
            std::uint8_t tag_byte = (static_cast<std::uint8_t>(tag)<<4) |
                                (static_cast<std::uint8_t>(TagType<TTag>()) << 2) |
                                static_cast<std::uint8_t>(0);
            m_descriptorArray.push_back(tag_byte);
        }

        template <class TTag, class TData>
        constexpr ShortItem(TTag tag, TData val)
        {
            const std::size_t SIZE = ValueSize(val);
            std::uint8_t tag_byte = (static_cast<std::uint8_t>(tag)<<4) |
                                (static_cast<std::uint8_t>(TagType<TTag>()) << 2) |
                                static_cast<std::uint8_t>((SIZE == 4U) ? 3U : SIZE);
            m_descriptorArray.push_back(tag_byte);

            auto value = static_cast<std::uint32_t>(val);
            for (std::size_t i = 0; i < SIZE; ++i)
            {
                m_descriptorArray.push_back(static_cast<std::uint8_t>(value));
                value >>= 8;
            }
        }
        constexpr CappedArray<std::uint8_t, 4+1> descriptor(DescriptorGlobalState&&) const
        {
            return m_descriptorArray;
        }

        constexpr       CappedArray<std::uint8_t, 4+1> &asArray()       { return m_descriptorArray; }
        constexpr const CappedArray<std::uint8_t, 4+1> &asArray() const { return m_descriptorArray; }
    protected:
        CappedArray<std::uint8_t, 4+1> m_descriptorArray;
    private:
    };


    template<TagMain MainType, int Value>
    class MainItem : public ShortItem<ValueSize<Value>()>
    {
    public:
        constexpr MainItem()
        : ShortItem<ValueSize<Value>()>{MainType, Value}
        {}

        using ctor_param_types = std::tuple<>;
        constexpr static TagMain tag = MainType;
        constexpr static int value = Value;
    protected:
    private:
    };

    template<int Value>
    class MainItem<TagMain::EndCollection, Value> : public ShortItem<0>
    {
    public:
        constexpr MainItem()
        : ShortItem<0>{TagMain::EndCollection}
        {}

        using ctor_param_types = std::tuple<>;
        constexpr static TagMain tag = TagMain::EndCollection;
        constexpr static int value = Value;
    };


    template<TagLocal LocalType, auto Value>
    class LocalItem : public ShortItem<ValueSize<Value>()>
    {
    public:
        constexpr LocalItem()
        : ShortItem<ValueSize<Value>()>{LocalType, Value}
        {}

        using ctor_param_types = std::tuple<>;

        constexpr int descriptorValue() const
        {
            return static_cast<int>(Value);
        }
    protected:
    private:
    };

    template<typename Page, Page Usage>
    class LocalItem<TagLocal::Usage, Usage> : public ShortItem<4>
    {
        using base = ShortItem<4>;
    public:
        constexpr LocalItem()
        : base{TagLocal::Usage, Usage}
        {}

        using ctor_param_types = std::tuple<>;

        constexpr int descriptorValue() const
        {
            return static_cast<int>(Usage);
        }

        constexpr CappedArray<std::uint8_t, 4 + 1> descriptor(DescriptorGlobalState &&globalState) const
        {
            CappedArray<std::uint8_t, 4 + 1> out;
            if (globalState.has(TagGlobal::UsagePage) && globalState.usage_page() == get_info<Page>().usagePage)
            {
                return base::descriptor(std::forward<DescriptorGlobalState>(globalState));
                // return ShortItem<4>{TagLocal::Usage, static_cast<uint16_t>(Usage)}.descriptor(std::forward<DescriptorGlobalState>(globalState));
            }
            uint32_t usageID = get_usage_id(Usage);
            return ShortItem<4>{TagLocal::Usage, usageID}.descriptor(std::forward<DescriptorGlobalState>(globalState));
        }
        static constexpr std::size_t descriptor_max_len = 5;
    protected:
    private:
        static constexpr Page s_usage = Usage;
    };


    template<TagGlobal GlobalType, auto Value>
    class GlobalItem : public ShortItem<ValueSize<Value>()>
    {
    public:
        constexpr GlobalItem()
        : ShortItem<ValueSize<Value>()>{GlobalType, Value}
        {}

        using ctor_param_types = std::tuple<>;
        using ctor_types = ctor_param_types;

        constexpr decltype(Value) descriptorValue() const
        {
            return Value;
        }

        constexpr CappedArray<std::uint8_t, ValueSize<Value>() + 1> descriptor(DescriptorGlobalState &&globalState) const
        {
            CappedArray<std::uint8_t, ValueSize<Value>() + 1> out;
            if (globalState.has(GlobalType) && globalState.value(GlobalType) == Value)
            {
                return out;
            }

            globalState.set(GlobalType, Value);
            out.insert(out.end(), this->m_descriptorArray.cbegin(), this->m_descriptorArray.cend());
            return out;
        }
    protected:
    private:
    };

    template<TagGlobal GlobalType, class RuntimeValueT>
    class DynamicGlobalItem : public ShortItem<std::dynamic_extent>
    {
    public:
        constexpr DynamicGlobalItem(RuntimeValueT runtimeValue)
        : ShortItem<std::dynamic_extent>{GlobalType, runtimeValue.value}
        , m_value{static_cast<int>(runtimeValue.value)}
        {}

        using ctor_param_types = std::tuple<RuntimeValueT>;
        constexpr static std::size_t descriptor_max_len = 5;
        constexpr static TagGlobal tag = GlobalType;

        constexpr int descriptorValue() const
        {
            return m_value;
        }

        constexpr CappedArray<std::uint8_t, descriptor_max_len> descriptor(DescriptorGlobalState &&globalState) const
        {
            CappedArray<std::uint8_t, descriptor_max_len> out;
            if (globalState.has(GlobalType) && globalState.value(GlobalType) == m_value)
            {
                return out;
            }

            globalState.set(GlobalType, m_value);
            out.insert(out.end(), this->m_descriptorArray.cbegin(), this->m_descriptorArray.cend());
            return out;
        }

    private:
        int m_value;
    };

    #define NAMED_TYPE(T, name) typedef struct name { T value; } name

    NAMED_TYPE(std::uint8_t, report_id_t);
    NAMED_TYPE(std::size_t, repeat_count_t);
    NAMED_TYPE(std::int32_t, logical_min_t);
    NAMED_TYPE(std::int32_t, logical_max_t);
    NAMED_TYPE(std::int32_t, physical_min_t);
    NAMED_TYPE(std::int32_t, physical_max_t);
    NAMED_TYPE(std::int32_t, report_count_t);
    NAMED_TYPE(std::int32_t, report_size_t);
    NAMED_TYPE(std::int32_t, unit_exponent_t);
    NAMED_TYPE(std::int32_t, unit_t);

    template<class reportID = OB::HID::ReportDescriptor::Helpers::Variable<uint8_t>> requires(OB::HID::ReportDescriptor::Helpers::is_value<reportID>::value)
    class ReportID : public ShortItem<1>
    {
    public:
        constexpr ReportID() requires(OB::HID::ReportDescriptor::Helpers::is_constant<reportID>::value)
        : ShortItem<1>(OB::HID::ReportDescriptor::TagGlobal::ReportID, reportID::value)
        , m_reportID{reportID::value}
        {}

        constexpr ReportID(report_id_t report_id) requires(OB::HID::ReportDescriptor::Helpers::is_variable<reportID>::value)
        : ShortItem<1>(OB::HID::ReportDescriptor::TagGlobal::ReportID, report_id.value)
        , m_reportID{report_id.value}
        {}

        using ctor_param_types = std::conditional_t<
            OB::HID::ReportDescriptor::Helpers::is_constant<reportID>::value, 
            std::tuple<>,
            std::tuple<report_id_t>
        >;

        static constexpr std::size_t dataSizeBits = 8;
        constexpr std::size_t sizeBits() const 
        { 
            return dataSizeBits;
        }

        constexpr int descriptorValue() const
        {
            return m_reportID;
        }

        constexpr uint8_t getReportID() const
        {
            return m_reportID;
        }

        constexpr CappedArray<std::uint8_t, 2> descriptor(DescriptorGlobalState &&globalState) const
        {
            CappedArray<std::uint8_t, 2> out;
            if (globalState.has(OB::HID::ReportDescriptor::TagGlobal::ReportID) && globalState.value(OB::HID::ReportDescriptor::TagGlobal::ReportID) == getReportID())
            {
                return out;
            }

            globalState.set(OB::HID::ReportDescriptor::TagGlobal::ReportID, getReportID());
            out.insert(out.end(), this->m_descriptorArray.cbegin(), this->m_descriptorArray.cend());
            return out;
        }

        constexpr report_id_t get(std::span<const uint8_t> reportData, std::size_t bitpos) const
        {
            assert(bitpos == 0); // ReportID must always be the first thing in the report.
            assert(reportData.size() > 0);
            return report_id_t{reportData[0]};
        }

        constexpr void set(std::span<uint8_t> reportBuffer, std::size_t bitpos) const
        {
            assert(bitpos == 0);
            assert(reportBuffer.size() > 0);
            reportBuffer[0] = m_reportID;
        }
    protected:
    private:
        const uint8_t m_reportID;
    };


    template<class Page>
    using UsagePage = GlobalItem<TagGlobal::UsagePage, OB::HID::get_info<Page>().usagePage>;
    // template<class Page> requires(OB::HID::ReportDescriptor::Helpers::is_value<Page>::value)
    // using UsagePage = std::conditional_t<
    //     OB::HID::ReportDescriptor::Helpers::is_constant<Page>::value,
    //     GlobalItem<TagGlobal::UsagePage, OB::HID::get_info<Page::value>().usagePage>,
    //     DynamicGlobalItem<TagGlobal::UsagePage, typename Page::type>
    // >;

    template<TagGlobal Tag, class Value>
    using _GlobalItem = GlobalItem<Tag, Value::value>;

    template<TagGlobal Tag, class T, class Value> requires(OB::HID::ReportDescriptor::Helpers::is_value<Value>::value)
    struct global_helper;
    template<TagGlobal Tag, class T, auto Value>
    struct global_helper<Tag, T, OB::HID::ReportDescriptor::Helpers::Constant<Value>>
    {
        using type = GlobalItem<Tag, Value>;
    };
    template<TagGlobal Tag, class T, class Type>
    struct global_helper<Tag, T, OB::HID::ReportDescriptor::Helpers::Variable<Type>>
    {
        using type = DynamicGlobalItem<Tag, T>;
    };


    #define GLOBAL_ITEM(name, _type)                          \
    template<class Value>                                    \
    using name = global_helper<TagGlobal::name, _type, Value>::type; 

    GLOBAL_ITEM(LogicalMinimum, logical_min_t);
    GLOBAL_ITEM(LogicalMaximum, logical_max_t);
    GLOBAL_ITEM(PhysicalMinimum, physical_min_t);
    GLOBAL_ITEM(PhysicalMaximum, physical_max_t);
    GLOBAL_ITEM(ReportCount, report_count_t);
    GLOBAL_ITEM(ReportSize, report_size_t);
    GLOBAL_ITEM(UnitExponent, unit_exponent_t);
    GLOBAL_ITEM(Unit, unit_t);


    template<auto Usage_>
    using Usage = LocalItem<TagLocal::Usage, Usage_>;

    template<auto Value>
    using UsageMinimum = LocalItem<TagLocal::UsageMinimum, static_cast<std::uint32_t>(Value)>;

    template<auto Value>
    using UsageMaximum = LocalItem<TagLocal::UsageMaximum, static_cast<std::uint32_t>(Value)>;



#pragma region Data Flags

    #define DATA_FLAG(a, b) enum class a##Or##b{a = 0, b = 1}

    DATA_FLAG(Data, Constant);
    DATA_FLAG(Array, Variable);
    DATA_FLAG(NoWrap, Wrap);
    DATA_FLAG(Linear, NonLinear);
    DATA_FLAG(PreferredState, NoPreferred);
    DATA_FLAG(NoNullPosition, NullState);
    DATA_FLAG(NonVolatile, Volatile);
    DATA_FLAG(BitField, BufferedBytes);

    template<auto Flag>
    constexpr uint32_t data_flag_value()
    {
        if constexpr (std::is_same_v<decltype(Flag), DataOrConstant>)
        {
            return static_cast<uint32_t>(Flag) << 0;
        }
        else if constexpr (std::is_same_v<decltype(Flag), ArrayOrVariable>)
        {
            return static_cast<uint32_t>(Flag) << 1;
        }
        else if constexpr (std::is_same_v<decltype(Flag), NoWrapOrWrap>)
        {
            return static_cast<uint32_t>(Flag) << 2;
        }
        else if constexpr (std::is_same_v<decltype(Flag), LinearOrNonLinear>)
        {
            return static_cast<uint32_t>(Flag) << 3;
        }
        else if constexpr (std::is_same_v<decltype(Flag), PreferredStateOrNoPreferred>)
        {
            return static_cast<uint32_t>(Flag) << 4;
        }
        else if constexpr (std::is_same_v<decltype(Flag), NoNullPositionOrNullState>)
        {
            return static_cast<uint32_t>(Flag) << 5;
        }
        else if constexpr (std::is_same_v<decltype(Flag), NonVolatileOrVolatile>)
        {
            return static_cast<uint32_t>(Flag) << 6;
        }
        else if constexpr (std::is_same_v<decltype(Flag), BitFieldOrBufferedBytes>)
        {
            return static_cast<uint32_t>(Flag) << 8;
        }
        else
        {
            static_assert(std::is_enum_v<decltype(Flag)>, "Unsupported data flag type");
            return 0;
        }
    }

    template<auto ...DataFlags_>
    struct DataFlags
    {
        static constexpr uint32_t value = (data_flag_value<DataFlags_>() | ... | 0U);
    };

#pragma endregion


#pragma region ItemGroup


    template<class A, class B>
    struct tuple_concat_types;

    template<class ...As, class ...Bs>
    struct tuple_concat_types<std::tuple<As...>, std::tuple<Bs...>>
    {
        using type = std::tuple<As..., Bs...>;
    };

    template<std::size_t Offset, std::size_t ...Is>
    constexpr std::index_sequence<(Offset + Is)...> add_group_offset(std::index_sequence<Is...>)
    {
        return {};
    }

    template<std::size_t Offset, class ...Items>
    struct _item_group_params_helper;

    template<std::size_t Offset>
    struct _item_group_params_helper<Offset>
    {
        using types = std::tuple<>;
        using indices = std::tuple<>;
    };

    template<std::size_t Offset, class Item, class ...Items>
    struct _item_group_params_helper<Offset, Item, Items...>
    {
    private:
        using item_types = typename Item::ctor_param_types;
        using item_count = std::tuple_size<item_types>;
        using rest = _item_group_params_helper<Offset + item_count::value, Items...>;

    public:
        using types = typename tuple_concat_types<item_types, typename rest::types>::type;
        using indices = decltype(
            std::tuple_cat(
                std::declval<std::tuple<decltype(add_group_offset<Offset>(std::make_index_sequence<item_count::value>{}))>>(),
                std::declval<typename rest::indices>()
            )
        );
    };

    template<class ...Items>
    class ItemGroup
    {
    public:
        static_assert((requires { Items::descriptor_max_len; } && ...), "All ItemGroup members must define descriptor_max_len");

        using ctor_param_types = typename _item_group_params_helper<0, Items...>::types;
        using ctor_indices = typename _item_group_params_helper<0, Items...>::indices;
        constexpr static std::size_t descriptor_max_len = (Items::descriptor_max_len + ... + 0);

        template<class Item, class TupleArgs, std::size_t ...AbsIs, std::size_t ...LocalIs>
        static constexpr Item make_group_item_impl(
            TupleArgs&& args,
            std::index_sequence<AbsIs...>,
            std::index_sequence<LocalIs...>)
        {
            using ctor_types = typename Item::ctor_param_types;
            return Item(
                std::tuple_element_t<LocalIs, ctor_types>{
                    std::get<AbsIs>(std::forward<TupleArgs>(args))
                }...
            );
        }

        template<class Item, class TupleArgs, class Indices>
        static constexpr Item make_group_item(TupleArgs&& args, Indices)
        {
            return make_group_item_impl<Item>(
                std::forward<TupleArgs>(args),
                Indices{},
                std::make_index_sequence<Indices::size()>{}
            );
        }

        template<class TupleArgs, class ...Indices>
        static constexpr std::tuple<Items...> make_group_items(
            TupleArgs&& args,
            std::tuple<Indices...>)
        {
            return std::tuple<Items...>{
                make_group_item<Items>(std::forward<TupleArgs>(args), Indices{})...
            };
        }

        constexpr ItemGroup() requires(std::tuple_size_v<ctor_param_types> == 0)
        : m_items(make_group_items(std::tuple<>{}, ctor_indices{}))
        {}

        template<typename ...TVals>
        requires(std::tuple_size_v<ctor_param_types> == sizeof...(TVals))
        constexpr ItemGroup(TVals&& ...vals)
        : m_items(make_group_items(
            std::forward_as_tuple(std::forward<TVals>(vals)...),
            ctor_indices{}
        ))
        {}

        constexpr const std::tuple<Items...>& items() const
        {
            return m_items;
        }

        constexpr std::tuple<Items...>& items()
        {
            return m_items;
        }

        template<std::size_t ...Is>
        constexpr CappedArray<std::uint8_t, descriptor_max_len> descriptor_impl(
            DescriptorGlobalState &&globalState,
            std::index_sequence<Is...>) const
        {
            CappedArray<std::uint8_t, descriptor_max_len> out;
            auto append_one = [&](const auto& child) {
                auto fragment = child.descriptor(std::forward<DescriptorGlobalState>(globalState));
                out.insert(out.end(), fragment.cbegin(), fragment.cend());
            };
            (append_one(std::get<Is>(m_items)), ...);
            return out;
        }

        constexpr CappedArray<std::uint8_t, descriptor_max_len> descriptor(DescriptorGlobalState&& globalState) const
        {
            return descriptor_impl(std::forward<DescriptorGlobalState>(globalState), std::make_index_sequence<sizeof...(Items)>{});
        }

        constexpr CappedArray<std::uint8_t, descriptor_max_len> asArray() const
        {
            DescriptorGlobalState state;
            return descriptor(state);
        }
    protected:
    private:
        std::tuple<Items...> m_items;
    };


    template<class DataFlags_, class ...LocalItems>
    class Input;

    template<auto ...DataFlags_, class ...LocalItems>
    class Input<DataFlags<DataFlags_...>, LocalItems...> : public ItemGroup<LocalItems..., MainItem<OB::HID::ReportDescriptor::TagMain::Input, DataFlags<DataFlags_...>::value>>
    {
        using flags = DataFlags<DataFlags_...>;
        using group_type = ItemGroup<LocalItems..., MainItem<OB::HID::ReportDescriptor::TagMain::Input, DataFlags<DataFlags_...>::value>>;
    public:
        using ctor_param_types = typename group_type::ctor_param_types;
        using group_type::group_type;
    protected:
    private:
    };

    template<class DataFlags_, class ...LocalItems>
    class Output;

    template<auto ...DataFlags_, class ...LocalItems>
    class Output<DataFlags<DataFlags_...>, LocalItems...> : public ItemGroup<LocalItems..., MainItem<OB::HID::ReportDescriptor::TagMain::Output, DataFlags<DataFlags_...>::value>>
    {
        using flags = DataFlags<DataFlags_...>;
        using group_type = ItemGroup<LocalItems..., MainItem<OB::HID::ReportDescriptor::TagMain::Output, DataFlags<DataFlags_...>::value>>;
    public:
        using ctor_param_types = typename group_type::ctor_param_types;
        using group_type::group_type;
    };

    template<class DataFlags_, class ...LocalItems>
    class Feature;

    template<auto ...DataFlags_, class ...LocalItems>
    class Feature<DataFlags<DataFlags_...>, LocalItems...> : public ItemGroup<LocalItems..., MainItem<OB::HID::ReportDescriptor::TagMain::Feature, DataFlags<DataFlags_...>::value>>
    {
        using flags = DataFlags<DataFlags_...>;
        using group_type = ItemGroup<LocalItems..., MainItem<OB::HID::ReportDescriptor::TagMain::Feature, DataFlags<DataFlags_...>::value>>;
    public:
        using ctor_param_types = typename group_type::ctor_param_types;
        using group_type::group_type;
    };


#pragma endregion


    template<CollectionType Type, class LeadingItem, class ...Items>
    class Collection : public ItemGroup<LeadingItem, MainItem<TagMain::Collection, static_cast<int>(Type)>, Items..., MainItem<TagMain::EndCollection, 0>>
    {
        using group_type = ItemGroup<LeadingItem, MainItem<TagMain::Collection, static_cast<int>(Type)>, Items..., MainItem<TagMain::EndCollection, 0>>;
    public:
        using ctor_param_types = typename group_type::ctor_param_types;
        using group_type::group_type;
    };


    template<std::size_t MIN, std::size_t MAX, class ...Items>
    class Repeat : public ItemGroup<Items...>
    {
        using group_type = ItemGroup<Items...>;
    public:
        static_assert(MIN <= MAX, "Repeat MIN must be <= MAX");

        using ctor_param_types = typename tuple_concat_types<std::tuple<repeat_count_t>, typename group_type::ctor_param_types>::type;
        constexpr static std::size_t descriptor_max_len = MAX * group_type::descriptor_max_len;

        template<typename ...TVals>
        requires(std::tuple_size_v<typename group_type::ctor_param_types> == sizeof...(TVals))
        constexpr Repeat(repeat_count_t repeatCount, TVals&& ...vals)
        : group_type(std::forward<TVals>(vals)...)
        , m_repeatCount(repeatCount.value)
        {
            assert(m_repeatCount >= MIN);
            assert(m_repeatCount <= MAX);
        }

        constexpr std::size_t repeat_count() const
        {
            return m_repeatCount;
        }

        constexpr CappedArray<std::uint8_t, descriptor_max_len> descriptor(DescriptorGlobalState &&globalState) const
        {
            CappedArray<std::uint8_t, descriptor_max_len> out;
            // Build one canonical repeated fragment so all repeated structures are identical.
            DescriptorGlobalState repeatState = globalState;
            auto repeatedFragment = group_type::descriptor(std::forward<DescriptorGlobalState>(repeatState));
            for (std::size_t i = 0; i < m_repeatCount; ++i)
            {
                out.insert(out.end(), repeatedFragment.cbegin(), repeatedFragment.cend());
            }
            globalState = repeatState;
            return out;
        }

    private:
        const std::size_t m_repeatCount;

    };

#pragma region helpers?

    template<std::size_t offset, class ...Items>
    struct _params_index_helper;

    template<std::size_t Offset>
    struct _params_index_helper<Offset>
    {
        using types = std::tuple<>;
        using indices = std::tuple<>;
        constexpr static std::size_t total_size = 0;
    };


    template <std::size_t Offset, std::size_t ... Is>
    std::index_sequence<(Offset + Is)...> add_offset(std::index_sequence<Is...>)
    {
        return {};
    }

    template<std::size_t Offset, class Item, class ...Items>
    struct _params_index_helper<Offset, Item, Items...>
    {
    private:
        using item_types   = typename Item::ctor_param_types;
        using item_count = std::tuple_size<item_types>;
        using rest = _params_index_helper<Offset + item_count::value, Items...>;
        using rest_types   = typename rest::types;
        using rest_indices = typename rest::indices;
        
    public:
        using types = decltype(
            std::tuple_cat(
                std::declval<item_types>(),
                std::declval<rest_types>()
            )
        );
        using indices = decltype(
            std::tuple_cat(
                std::declval<
                    std::tuple<
                        decltype(add_offset<Offset>(std::make_index_sequence<item_count::value>()))
                    >
                >(),
                std::declval<rest_indices>()
            )
        );
        constexpr static std::size_t total_size = 
            Item::descriptor_max_len + 
            rest::total_size;
    };

    template<class ...Items>
    struct params_list : _params_index_helper<0, Items...>{};

    template<class U, class V>
    struct is_compatible : 
        std::bool_constant<std::is_convertible<U, V>::value || std::is_constructible<U, V>::value>
    { };

    template<>
    struct is_compatible<std::tuple<>, std::tuple<>> : std::true_type
    { };

    template<class U, class ...Us, class V, class ...Vs>
    struct is_compatible<std::tuple<U, Us...>, std::tuple<V, Vs...>> : 
        std::bool_constant<
            is_compatible<U, V>::value &&
            is_compatible<std::tuple<Us...>, std::tuple<Vs...>>::value
        >
    { };


    template<class T>
    struct global_item_info
    {
        constexpr static bool is_global = false;
    };

    template<TagGlobal GlobalType, auto Value>
    struct global_item_info<GlobalItem<GlobalType, Value>>
    {
        constexpr static bool is_global = true;
        constexpr static TagGlobal tag = GlobalType;
        constexpr static auto static_value = Value;
    };

    template<class reportID>
    struct global_item_info<ReportID<reportID>>
    {
        constexpr static bool is_global = true;
        constexpr static TagGlobal tag = TagGlobal::ReportID;
    };

    template<TagGlobal GlobalType, class RuntimeValueT>
    struct global_item_info<DynamicGlobalItem<GlobalType, RuntimeValueT>>
    {
        constexpr static bool is_global = true;
        constexpr static TagGlobal tag = GlobalType;
    };

    template<class T>
    struct local_item_info
    {
        constexpr static bool is_local = false;
    };

    template<TagLocal LocalType, auto Value>
    struct local_item_info<LocalItem<LocalType, Value>>
    {
        constexpr static bool is_local = true;
        constexpr static TagLocal tag = LocalType;
        constexpr static auto static_value = Value;
    };

    template<class T>
    struct main_item_info
    {
        constexpr static bool is_main = false;
    };

    template<TagMain MainType, int Value>
    struct main_item_info<MainItem<MainType, Value>>
    {
        constexpr static bool is_main = true;
        constexpr static TagMain tag = MainType;
    };

#pragma endregion

}