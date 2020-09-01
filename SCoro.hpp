#pragma once
#include <type_traits>
#include <utility>
#include <memory>

namespace SCoro
{
    namespace Impl
    {
        struct Nothing{};

        template <size_t I>
        struct Index{};

        template <template <typename> class ... Args>
        struct Stages;

        template <template <typename> class ... Args>
        struct ArgList{};

        template <size_t count, template <typename> class ...>
        struct StagesImpl;

        template <size_t count, template <typename> class Arg>
        struct StagesImpl<count, Arg> : Arg<Nothing>
        {
            using base = Arg<Nothing>;
            using base::base;
        private:
            static constexpr size_t index = 0;
        public:
            constexpr StagesImpl & get_at(Index<index>) noexcept { return *this; }
            constexpr StagesImpl const & get_at(Index<index>) const noexcept { return *this; }
        };

        template <size_t count, template <typename> class Arg, template <typename> class ... Args>
        struct StagesImpl<count, Arg, Args...> : Arg<StagesImpl<count, Args...>>
        {
            using base = Arg<StagesImpl<count, Args...>>;
            using base::base;
        private:
            static constexpr size_t index = sizeof...(Args);
        public:
            using base::get_at;
            constexpr StagesImpl & get_at(Index<index>) noexcept { return *this; }
            constexpr StagesImpl const & get_at(Index<index>) const noexcept { return *this; }
        };

        template <typename Base, typename Input, typename Output = ArgList<>>
        struct ReverseStages;

        template <typename Base, template <typename> class ... Args>
        struct ReverseStages<Base, ArgList<>, ArgList<Args...>>
        {
            using type = StagesImpl<sizeof...(Args), Args...>;
        };

        template <typename Base, template <typename> class T, template <typename> class ... Ts, template <typename> class  ... Args>
        struct ReverseStages<Base, ArgList<T, Ts...>, ArgList<Args...>>
        {
            using type = typename ReverseStages<Base, ArgList<Ts...>, ArgList<T, Args...>>::type;
        };

        template <size_t index, typename Stgs>
        constexpr static auto poll_fn(Stgs & self) noexcept
        {
            return self.get_at(Index<index>{}).Poll();
        }

        template <typename Stgs, typename>
        struct GetImpl;
        template <typename Stgs, size_t ... I>
        struct GetImpl<Stgs, std::index_sequence<I...>>
        {
            using common_fn_t = std::common_type_t<decltype(poll_fn<I, Stgs>)...>;
            static constexpr common_fn_t lut[]{ poll_fn<I, Stgs>... };
            static constexpr auto get(Stgs & stack) noexcept
            {
                return lut[stack.index];
            }
        };

        template <typename Stgs>
        constexpr auto get(Stgs & stack) noexcept
        {
            return GetImpl<Stgs, std::make_index_sequence<Stgs::count>>::get(stack);
        }
    }

    template <template <typename> class ... Args>
    struct SCoro : Impl::ReverseStages<SCoro<Args...>, Impl::ArgList<Args...>>::type
    {
        using base = typename Impl::ReverseStages<SCoro<Args...>, Impl::ArgList<Args...>>::type;
        using base::base;
        using base::get_at;
        static constexpr size_t count = sizeof...(Args);

        mutable size_t index = 0;
        constexpr void Reset() noexcept
        {
            this->~SCoro(); 
            new (this) SCoro{};
        }
        constexpr bool Done() const noexcept
        {
            return index == count;
        }
        constexpr bool Poll() noexcept
        {
            if (Impl::get(*this)(*this)) ++index;
            return index < count;
        }
    };
}
