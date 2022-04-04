#pragma once

#include "cc/utility.h"
#include "cc/http_request.h"
#include "cc/http_response.h"

namespace cc {
  namespace detail {
	template <typename ... Middlewares>
	struct partial_context
	  : public spell::pop_back<Middlewares...>::template rebind<partial_context>
	  ,public spell::last_element_type<Middlewares...>::type::Ctx
	{
		using parent_context=typename spell::pop_back<Middlewares...>::template rebind<::cc::detail::partial_context>;
		template <int N>
		using partial=typename std::conditional<N==sizeof...(Middlewares)-1, partial_context, typename parent_context::template partial<N>>::type;

		template <typename T>
		typename T::Ctx& get() {
			return static_cast<typename T::Ctx&>(*this);
		}
	};

	template <>
	struct partial_context<> {
	  template <int>
	  using partial=partial_context;
	};

	template <int N,typename Context,typename Container,typename CurrentMW,typename ... Middlewares>
	bool middleware_call_helper(Container& middlewares,Req& req,Res& res,Context& ctx);

	template <typename ... Middlewares>
	struct Ctx : private partial_context<Middlewares...>
	  //struct context : private Middlewares::Ctx... // simple but less type-safe
	{
	  template <int N,typename Context,typename Container>
	  friend typename std::enable_if<(N==0)>::type after_handlers_call_helper(Container& middlewares,Context& ctx,Req& req,Res& res);
	  template <int N,typename Context,typename Container>
	  friend typename std::enable_if<(N>0)>::type after_handlers_call_helper(Container& middlewares,Context& ctx,Req& req,Res& res);

	  template <int N,typename Context,typename Container,typename CurrentMW,typename ... Middlewares2>
	  friend bool middleware_call_helper(Container& middlewares,Req& req,Res& res,Context& ctx);

	  template <typename T>
	  typename T::Ctx& get() {
		return static_cast<typename T::Ctx&>(*this);
	  }

	  template <int N>
	  using partial=typename partial_context<Middlewares...>::template partial<N>;
	};
  }
}
