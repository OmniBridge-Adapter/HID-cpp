#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cassert>
#include <limits>
#include <type_traits>
#include <span>
#include <tuple>

#include "hid/usage.hpp"
#include "hid/report_descriptor/item.hpp"
#include "hid/report_descriptor.hpp"

#include "capped_array.hpp"
#include "bitspan.hpp"

namespace OB::HID
{
    using namespace ReportDescriptor;

    template<std::size_t Offset, typename T, std::size_t... Is>
    constexpr auto make_from_slice(
        T&& tup,
        std::index_sequence<Is...>)
    {
        return std::tuple{
            std::get<Offset + Is>(std::forward<T>(tup))...
        };
    }

    template<typename Item,
            typename TupleArgs,
            std::size_t ...Is>
    constexpr Item 
    make_component_impl(
        TupleArgs&& args,
        std::index_sequence<Is...>)
    {
        using ctor_types = typename Item::ctor_param_types;
        return [&]<std::size_t ...LocalIs>(std::index_sequence<LocalIs...>) {
            return Item(
                std::tuple_element_t<LocalIs, ctor_types>{
                    std::get<Is>(std::forward<TupleArgs>(args))
                }...
            );
        }(std::make_index_sequence<sizeof...(Is)>{});
    }

    template<typename Item,
            typename TupleArgs,
            typename Is>
    constexpr Item 
    make_component(
        TupleArgs&& args,
        Is is)
    {
        return make_component_impl<Item>(std::forward<TupleArgs>(args), is);
    }

    template <class ...Items>
    class Report
    {
        static_assert((requires { Items::descriptor_max_len; } && ...), "All Report items must define descriptor_max_len");

        struct UsageToken
        {
            int usagePage;
            int usage;
        };

        using ctor_types = params_list<Items...>::types;
        using ctor_indices = params_list<Items...>::indices;
        static constexpr std::size_t MAX_SIZE = (Items::descriptor_max_len + ...);


        template<typename TupleArgs, typename ...Indices>
        static constexpr std::tuple<Items...> make_report_items(
            TupleArgs&& args,
            std::tuple<Indices...>)
        {
            return std::tuple<Items...>{
                make_component<Items>(std::forward<TupleArgs>(args), Indices{})...
            };
        }

        template<std::size_t ...Is>
        constexpr CappedArray<uint8_t, MAX_SIZE> descriptor_impl(std::index_sequence<Is...>) const
        {
            CappedArray<uint8_t, MAX_SIZE> out;
            DescriptorGlobalState globalState;
            auto append_one = [&](const auto& item) {
                auto fragment = item.descriptor_fragment(globalState);
                out.insert(out.end(), fragment.cbegin(), fragment.cend());
            };
            (append_one(std::get<Is>(m_reportItems)), ...);

            return out;
        }

        struct LayoutWalkState
        {
            int usagePage = 0;
            std::size_t reportSizeBits = 0;
            std::size_t reportCount = 0;
            std::size_t bitOffset = 0;
            bool hasReportID = false;

            bool localUsageValid = false;
            int localUsage = 0;
            bool localUsageMinValid = false;
            int localUsageMin = 0;
            bool localUsageMaxValid = false;
            int localUsageMax = 0;

            std::array<UsageToken, 16> collectionStack{};
            std::size_t collectionDepth = 0;
        };

        struct UsageFieldSearch
        {
            std::array<UsageToken, 16> path{};
            std::size_t pathLen = 0;
            std::size_t remainingPathInstance = 0;
            bool found = false;
            std::size_t bitOffset = 0;
            std::size_t bitCount = 0;
        };

        template<class T, class = void>
        struct has_items_method : std::false_type
        {};

        template<class T>
        struct has_items_method<T, std::void_t<decltype(std::declval<const T&>().items())>> : std::true_type
        {};

        template<class T, class = void>
        struct has_repeat_count_method : std::false_type
        {};

        template<class T>
        struct has_repeat_count_method<T, std::void_t<decltype(std::declval<const T&>().repeat_count())>> : std::true_type
        {};

        constexpr void reset_local_state(LayoutWalkState& state) const
        {
            state.localUsageValid = false;
            state.localUsage = 0;
            state.localUsageMinValid = false;
            state.localUsageMin = 0;
            state.localUsageMaxValid = false;
            state.localUsageMax = 0;
        }

        constexpr bool matches_usage_path(
            const LayoutWalkState& state,
            const UsageFieldSearch& search,
            int leafUsagePage,
            int leafUsage) const
        {
            if (search.pathLen == 0)
            {
                return false;
            }

            const std::size_t collectionPathLen = search.pathLen - 1;
            if (state.collectionDepth < collectionPathLen)
            {
                return false;
            }

            for (std::size_t i = 0; i < collectionPathLen; ++i)
            {
                if (state.collectionStack[i].usage != search.path[i].usage)
                {
                    return false;
                }
            }

            const auto& leaf = search.path[search.pathLen - 1];
            return leaf.usagePage == leafUsagePage && leaf.usage == leafUsage;
        }

        constexpr void consider_usage_field(
            LayoutWalkState& state,
            UsageFieldSearch* search,
            int usage,
            std::size_t fieldBitOffset,
            std::size_t fieldBitCount) const
        {
            if (search == nullptr)
            {
                return;
            }
            if (search->found)
            {
                return;
            }
            if (!matches_usage_path(state, *search, state.usagePage, usage))
            {
                return;
            }

            if (search->remainingPathInstance > 0)
            {
                search->remainingPathInstance--;
                return;
            }

            search->found = true;
            search->bitOffset = fieldBitOffset;
            search->bitCount = fieldBitCount;
        }

        constexpr void handle_main_item(LayoutWalkState& state, TagMain tag, UsageFieldSearch* search) const
        {
            if (tag == TagMain::Collection)
            {
                assert(state.collectionDepth < state.collectionStack.size());
                state.collectionStack[state.collectionDepth] = {
                    state.localUsageValid ? state.usagePage : -1,
                    state.localUsageValid ? state.localUsage : -1,
                };
                state.collectionDepth++;
                reset_local_state(state);
                return;
            }

            if (tag == TagMain::EndCollection)
            {
                assert(state.collectionDepth > 0);
                state.collectionDepth--;
                reset_local_state(state);
                return;
            }

            if (tag != TagMain::Input && tag != TagMain::Output)
            {
                reset_local_state(state);
                return;
            }

            for (std::size_t i = 0; i < state.reportCount; ++i)
            {
                const std::size_t fieldBitOffset = state.bitOffset + (i * state.reportSizeBits);
                int usage = -1;
                if (state.localUsageMinValid && state.localUsageMaxValid)
                {
                    usage = state.localUsageMin + static_cast<int>(i);
                }
                else if (state.localUsageValid)
                {
                    usage = state.localUsage + static_cast<int>(i);
                }

                if (usage >= 0)
                {
                    consider_usage_field(state, search, usage, fieldBitOffset, state.reportSizeBits);
                }
            }

            state.bitOffset += state.reportCount * state.reportSizeBits;
            reset_local_state(state);
        }

        template<class Item>
        constexpr void walk_layout_item(const Item& item, LayoutWalkState& state, UsageFieldSearch* search) const
        {
            if constexpr (has_items_method<Item>::value)
            {
                constexpr std::size_t itemCount = std::tuple_size_v<std::remove_cvref_t<decltype(item.items())>>;
                if constexpr (has_repeat_count_method<Item>::value)
                {
                    for (std::size_t i = 0; i < item.repeat_count(); ++i)
                    {
                        walk_layout_tuple(item.items(), state, search, std::make_index_sequence<itemCount>{});
                    }
                }
                else
                {
                    walk_layout_tuple(item.items(), state, search, std::make_index_sequence<itemCount>{});
                }
                return;
            }

            if constexpr (global_item_info<Item>::is_global)
            {
                constexpr auto tag = global_item_info<Item>::tag;
                const int value = item.descriptorValue();
                if (tag == TagGlobal::UsagePage)
                {
                    state.usagePage = value;
                }
                else if (tag == TagGlobal::ReportSize)
                {
                    assert(value >= 0);
                    state.reportSizeBits = static_cast<std::size_t>(value);
                }
                else if (tag == TagGlobal::ReportCount)
                {
                    assert(value >= 0);
                    state.reportCount = static_cast<std::size_t>(value);
                }
                else if (tag == TagGlobal::ReportID)
                {
                    if (!state.hasReportID)
                    {
                        state.bitOffset += 8;
                        state.hasReportID = true;
                    }
                }
                return;
            }

            if constexpr (local_item_info<Item>::is_local)
            {
                constexpr auto tag = local_item_info<Item>::tag;
                const int value = item.descriptorValue();
                if (tag == TagLocal::Usage)
                {
                    state.localUsageValid = true;
                    state.localUsage = value;
                }
                else if (tag == TagLocal::UsageMinimum)
                {
                    state.localUsageMinValid = true;
                    state.localUsageMin = value;
                }
                else if (tag == TagLocal::UsageMaximum)
                {
                    state.localUsageMaxValid = true;
                    state.localUsageMax = value;
                }
                return;
            }

            if constexpr (main_item_info<Item>::is_main)
            {
                handle_main_item(state, main_item_info<Item>::tag, search);
            }
        }

        template<class Tuple, std::size_t ...Is>
        constexpr void walk_layout_tuple(const Tuple& tuple, LayoutWalkState& state, UsageFieldSearch* search, std::index_sequence<Is...>) const
        {
            (walk_layout_item(std::get<Is>(tuple), state, search), ...);
        }

        constexpr std::size_t report_size_bits() const
        {
            LayoutWalkState state{};
            walk_layout_tuple(m_reportItems, state, nullptr, std::make_index_sequence<sizeof...(Items)>{});
            return state.bitOffset;
        }

        constexpr UsageFieldSearch find_usage_field_by_path(
            std::span<const UsageToken> path,
            std::size_t pathInstance) const
        {
            UsageFieldSearch search{};
            assert(path.size() > 0);
            assert(path.size() <= search.path.size());
            search.pathLen = path.size();
            search.remainingPathInstance = pathInstance;
            for (std::size_t i = 0; i < path.size(); ++i)
            {
                search.path[i] = path[i];
            }

            LayoutWalkState state{};
            walk_layout_tuple(m_reportItems, state, &search, std::make_index_sequence<sizeof...(Items)>{});
            return search;
        }

        template<auto UsageValue>
        static constexpr UsageToken make_path_token()
        {
            return UsageToken{
                OB::HID::get_info<std::remove_cvref_t<decltype(UsageValue)>>().usagePage,
                static_cast<int>(UsageValue),
            };
        }

        template<auto ...UsagePath>
        static constexpr auto make_path_tokens()
        {
            return std::array<UsageToken, sizeof...(UsagePath)>{
                make_path_token<UsagePath>()...
            };
        }

    public:
        template<typename ...TVals>
        requires(is_compatible<ctor_types, std::tuple<std::remove_cvref_t<TVals>...>>::value)
        constexpr Report(TVals&& ...vals)
        : m_reportItems(make_report_items(
            std::forward_as_tuple(std::forward<TVals>(vals)...),
            ctor_indices{}
        ))
        {}

        constexpr CappedArray<uint8_t, MAX_SIZE> descriptor() const
        {
            return descriptor_impl(std::make_index_sequence<sizeof...(Items)>{});
        }
        constexpr std::size_t descriptorSize() const 
        { 
            return descriptor().size(); 
        }
        constexpr std::size_t reportSize() const
        {
            const std::size_t bits = report_size_bits();
            return (bits + 7) / 8;
        }

        template<auto ...UsagePath>
        constexpr uint32_t get(std::span<const uint8_t> reportData, std::size_t pathInstance = 0) const
        {
            static_assert(sizeof...(UsagePath) > 0, "get requires at least one usage in the path");
            constexpr auto path = make_path_tokens<UsagePath...>();
            const auto field = find_usage_field_by_path(std::span{path}, pathInstance);
            assert(field.found);
            assert(field.bitCount <= 32);

            bitspan<const uint8_t> bits{reportData};
            return bits.template get<uint32_t>(field.bitOffset, field.bitCount);
        }

        template<auto ...UsagePath, typename T>
        constexpr void set(std::span<uint8_t> reportData, T value, std::size_t pathInstance = 0) const
        {
            static_assert(sizeof...(UsagePath) > 0, "set requires at least one usage in the path");
            constexpr auto path = make_path_tokens<UsagePath...>();
            const auto field = find_usage_field_by_path(std::span{path}, pathInstance);
            assert(field.found);

            bitspan<uint8_t> bits{reportData};
            bits.template set<T>(value, field.bitOffset, field.bitCount);
        }

    protected:
    private:
        std::tuple<Items...> m_reportItems;
    };
}

/*

using MyReport = Report<
    Items::Global::ReportID<-1>, // -1 is default, dynamic
    Items::Global::UsagePage<HID::Page::GenericDesktop>,
    Items::Main::Collection<
        CollectionType::Logical,
        Items::Local::Usage<HID::Page::GenericDesktop::Keyboard>,
        Items::Main::Input<
            DataFlags<
                DataOrConstant::Data,
                ArrayOrVariable::Variable,
                NoWrapOrWrap::NoWrap,
                LinearOrNonLinear::Linear,
                PreferredStateOrNoPreferred::NoPreferred,
                NoNullPositionOrNullState::NoNullPosition,
                NonVolatileOrVolatile::Volatile,
                BitFieldOrBufferedBytes::BitField,
            >,
            Items::Global::UsagePage<HID::Page::KeyCodes>,
            Items::Local::UsageMinimum<HID::Page::KeyCodes::224>,
            Items::Local::UsageMaximum<HID::Page::KeyCodes::231>,
            Items::Local::ReportCount<8>,
            Items::Local::ReportSize<1>,
            Items::Global::LogicalMinimum<0>,
            Items::Global::LogicalMaximum<1>,
        >
    >,
    
>;

MyReport myReportInstance{
    report_id_t{reportID}
};

std::array<uint8_t, MyReport::size()> myReportBuf;

constexpr auto myReportDescriptor = myReportInstance.descriptor();

myReportInstance.set<ReportID>(myReportBuf);
myReportInstance.set<HID::Page::GenericDesktop::Keyboard, HID::Page::KeyCodes::224>(0)
myReportInstance.set<HID::Page::GenericDesktop::Keyboard, HID::Page::KeyCodes::225>(0)
myReportInstance.set<HID::Page::GenericDesktop::Keyboard, HID::Page::KeyCodes::226>(0)
myReportInstance.set<HID::Page::GenericDesktop::Keyboard, HID::Page::KeyCodes::227>(0)






*/
