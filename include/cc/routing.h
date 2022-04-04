#pragma once
#include <cstdint>
#include <utility>
#include <tuple>
#include <unordered_map>
#include <memory>
#include <boost/lexical_cast.hpp>
#include <vector>
#include "cc/common.h"
#include "cc/http_response.h"
#include "cc/http_request.h"
#include "cc/utility.h"
#include "cc/logging.h"
#include "cc/websocket.h"
namespace cc {
  /// A base class for all rules.
  /// Used to provide a common interface for code dealing with different types of rules.
  /// A Rule provides a URL, allowed HTTP methods, and handlers.
  class BaseRule {
    public:
    BaseRule(std::string rule) : rule_(std::move(rule)) {}

    virtual ~BaseRule() {}

    virtual void validate()=0;
    std::unique_ptr<BaseRule> upgrade() {
      if (rule_to_upgrade_)
        return std::move(rule_to_upgrade_);
      return {};
    }

    virtual void handle(const Req&,Res&,const routing_params&)=0;
    virtual void handle_upgrade(const Req&,Res& res,SocketAdaptor&&) {
      res=Res(404);
      res.end();
    }
#ifdef ENABLE_SSL
    virtual void handle_upgrade(const Req&,Res& res,SSLAdaptor&&) {
      res=Res(404);
      res.end();
    }
#endif

    uint32_t get_methods() { return methods_; }
    template <typename F>
    void foreach_method(F f) {
      for (uint32_t method=0,method_bit=1; method<static_cast<uint32_t>(HTTP::InternalMethodCount); ++method,method_bit<<=1) {
        if (methods_&method_bit)
          f(method);
      }
    }

    const std::string& rule() { return rule_; }

    protected:
    uint32_t methods_{1<<static_cast<int>(HTTP::GET)};
    std::string rule_;
    std::string name_;
    std::unique_ptr<BaseRule> rule_to_upgrade_;
    friend class Router;
    template <typename T>
    friend struct RuleParameterTraits;
  };

  namespace detail {
    namespace routing_handler_call_helper {
      template <typename T,int Pos>
      struct call_pair {
        using type=T;
        static const int pos=Pos;
      };

      template <typename H1>
      struct call_params {
        H1& handler;
        const routing_params& params;
        const Req& req;
        Res& res;
      };

      template <typename F,int NInt,int NUint,int NDouble,int NString,typename S1,typename S2>
      struct call {};

      template <typename F,int NInt,int NUint,int NDouble,int NString,typename ... Args1,typename ... Args2>
      struct call<F,NInt,NUint,NDouble,NString,spell::S<int64_t,Args1...>,spell::S<Args2...>> {
        void operator()(F cparams) {
          using pushed=typename spell::S<Args2...>::template push_back<call_pair<int64_t,NInt>>;
          call<F,NInt+1,NUint,NDouble,NString,
            spell::S<Args1...>,pushed>()(cparams);
        }
      };

      template <typename F,int NInt,int NUint,int NDouble,int NString,typename ... Args1,typename ... Args2>
      struct call<F,NInt,NUint,NDouble,NString,spell::S<uint64_t,Args1...>,spell::S<Args2...>> {
        void operator()(F cparams) {
          using pushed=typename spell::S<Args2...>::template push_back<call_pair<uint64_t,NUint>>;
          call<F,NInt,NUint+1,NDouble,NString,
            spell::S<Args1...>,pushed>()(cparams);
        }
      };

      template <typename F,int NInt,int NUint,int NDouble,int NString,typename ... Args1,typename ... Args2>
      struct call<F,NInt,NUint,NDouble,NString,spell::S<double,Args1...>,spell::S<Args2...>> {
        void operator()(F cparams) {
          using pushed=typename spell::S<Args2...>::template push_back<call_pair<double,NDouble>>;
          call<F,NInt,NUint,NDouble+1,NString,
            spell::S<Args1...>,pushed>()(cparams);
        }
      };

      template <typename F,int NInt,int NUint,int NDouble,int NString,typename ... Args1,typename ... Args2>
      struct call<F,NInt,NUint,NDouble,NString,spell::S<std::string,Args1...>,spell::S<Args2...>> {
        void operator()(F cparams) {
          using pushed=typename spell::S<Args2...>::template push_back<call_pair<std::string,NString>>;
          call<F,NInt,NUint,NDouble,NString+1,
            spell::S<Args1...>,pushed>()(cparams);
        }
      };

      template <typename F,int NInt,int NUint,int NDouble,int NString,typename ... Args1>
      struct call<F,NInt,NUint,NDouble,NString,spell::S<>,spell::S<Args1...>> {
        void operator()(F cparams) {
          cparams.handler(
            cparams.req,
            cparams.res,
            cparams.params.template get<typename Args1::type>(Args1::pos)...
          );
        }
      };

      template <typename Func,typename ... ArgsWrapped>
      struct Wrapped {
        template <typename ... Args>
        void set_(Func f,typename std::enable_if<
                  !std::is_same<typename std::tuple_element<0,std::tuple<Args...,void>>::type,const Req&>::value
                  ,int>::type=0) {
          handler_=(
          [f=std::move(f)]
          (const Req&,Res&res,Args... args){
            res=Res(f(args...));
            res.end();
          });
        }

        template <typename Req,typename ... Args>
        struct req_handler_wrapper {
          req_handler_wrapper(Func f)
            : f(std::move(f)) {}

          void operator()(const Req& req,Res& res,Args... args) {
            res=Res(f(req,args...));
            res.end();
          }

          Func f;
        };

        template <typename ... Args>
        void set_(Func f,typename std::enable_if<
                  std::is_same<typename std::tuple_element<0,std::tuple<Args...,void>>::type,const Req&>::value&&
                  !std::is_same<typename std::tuple_element<1,std::tuple<Args...,void,void>>::type,Res&>::value
                  ,int>::type=0) {
          handler_=req_handler_wrapper<Args...>(std::move(f));
          /*handler_ = (
              [f = std::move(f)]
              (const Req& req, Res& res, Args... args){
                   res = Res(f(req, args...));
                   res.end();
              });*/
        }

        template <typename ... Args>
        void set_(Func f,typename std::enable_if<
                  std::is_same<typename std::tuple_element<0,std::tuple<Args...,void>>::type,const Req&>::value &&
                  std::is_same<typename std::tuple_element<1,std::tuple<Args...,void,void>>::type,Res&>::value
                  ,int>::type=0) {
          handler_=std::move(f);
        }

        template <typename ... Args>
        struct handler_type_helper {
          using type=std::function<void(const cc::Req&,cc::Res&,Args...)>;
          using args_type=spell::S<typename spell::promote_t<Args>...>;
        };

        template <typename ... Args>
        struct handler_type_helper<const Req&,Args...> {
          using type=std::function<void(const cc::Req&,cc::Res&,Args...)>;
          using args_type=spell::S<typename spell::promote_t<Args>...>;
        };

        template <typename ... Args>
        struct handler_type_helper<const Req&,Res&,Args...> {
          using type=std::function<void(const cc::Req&,cc::Res&,Args...)>;
          using args_type=spell::S<typename spell::promote_t<Args>...>;
        };

        typename handler_type_helper<ArgsWrapped...>::type handler_;

        void operator()(const Req& req,Res& res,const routing_params& params) {
          detail::routing_handler_call_helper::call<
            detail::routing_handler_call_helper::call_params<
            decltype(handler_)>,
            0,0,0,0,
            typename handler_type_helper<ArgsWrapped...>::args_type,
            spell::S<>
          >()(
            detail::routing_handler_call_helper::call_params<
            decltype(handler_)>
          {handler_,params,req,res}
          );
        }
      };

    }
  }

  class CatchallRule {
    public:
    CatchallRule() {}

    template <typename Func>
    typename std::enable_if<spell::CallHelper<Func,spell::S<>>::value,void>::type
      operator()(Func&& f) {
      static_assert(!std::is_same<void,decltype(f())>::value,
                    "Handler function cannot have void return type; valid return types: string, int, cc::Res, cc::returnable");

      handler_=(
      [f=std::move(f)]
      (const Req&,Res&res){
        res=Res(f());
        res.end();
      });

    }

    template <typename Func>
    typename std::enable_if<
      !spell::CallHelper<Func,spell::S<>>::value &&
      spell::CallHelper<Func,spell::S<cc::Req>>::value,
      void>::type
      operator()(Func&& f) {
      static_assert(!std::is_same<void,decltype(f(std::declval<cc::Req>()))>::value,
                    "Handler function cannot have void return type; valid return types: string, int, cc::Res, cc::returnable");

      handler_=(
      [f=std::move(f)]
      (const cc::Req&req,cc::Res&res){
        res=Res(f(req));
        res.end();
      });
    }

    template <typename Func>
    typename std::enable_if<
      !spell::CallHelper<Func,spell::S<>>::value&&
      !spell::CallHelper<Func,spell::S<cc::Req>>::value &&
      spell::CallHelper<Func,spell::S<cc::Res&>>::value,
      void>::type
      operator()(Func&& f) {
      static_assert(std::is_same<void,decltype(f(std::declval<cc::Res&>()))>::value,
                    "Handler function with Res argument should have void return type");
      handler_=(
      [f=std::move(f)]
      (const cc::Req&,cc::Res&res){
        f(res);
      });
    }

    template <typename Func>
    //typename std::enable_if<spell::CallHelper<Func,spell::S<>>::value,void>::type
    typename std::enable_if<
      !spell::CallHelper<Func,spell::S<>>::value&&
      !spell::CallHelper<Func,spell::S<cc::Req>>::value&&
      !spell::CallHelper<Func,spell::S<cc::Res&>>::value,
      void>::type
      operator()(Func&& f) {
      static_assert(!std::is_same<void,decltype(f())>::value,
                    "Handler function with Res argument should have other return type");

      handler_=(
        [f=std::move(f)]
        (const Req&req,Res&res){
          res=Res(f(req,res));
          res.end();
        });
    }

    bool has_handler() {
      return (handler_!=nullptr);
    }

    protected:
    friend class Router;
    private:
    std::function<void(const cc::Req&,cc::Res&)> handler_;
  };

  /// A rule dealing with websockets.
  /// Provides the interface for the user to put in the necessary handlers for a websocket to work.
  class WebSocketRule : public BaseRule {
    using self_t=WebSocketRule;
    public:
    WebSocketRule(std::string rule)
      : BaseRule(std::move(rule)) {}

    void validate() override {}

    void handle(const Req&,Res& res,const routing_params&) override {
      res=Res(404);
      res.end();
    }

    void handle_upgrade(const Req& req,Res&,SocketAdaptor&& adaptor) override {
      new cc::websocket::Connection<SocketAdaptor>(req,std::move(adaptor),open_handler_,message_handler_,close_handler_,error_handler_,accept_handler_);
    }
#ifdef ENABLE_SSL
    void handle_upgrade(const Req& req,Res&,SSLAdaptor&& adaptor) override {
      new cc::websocket::Connection<SSLAdaptor>(req,std::move(adaptor),open_handler_,message_handler_,close_handler_,error_handler_,accept_handler_);
    }
#endif

    template <typename Func>
    self_t& onopen(Func f) {
      open_handler_=f;
      return *this;
    }

    template <typename Func>
    self_t& onmessage(Func f) {
      message_handler_=f;
      return *this;
    }

    template <typename Func>
    self_t& onclose(Func f) {
      close_handler_=f;
      return *this;
    }

    template <typename Func>
    self_t& onerror(Func f) {
      error_handler_=f;
      return *this;
    }

    template <typename Func>
    self_t& onaccept(Func f) {
      accept_handler_=f;
      return *this;
    }

    protected:
    std::function<void(cc::websocket::connection&)> open_handler_;
    std::function<void(cc::websocket::connection&,const std::string&,bool)> message_handler_;
    std::function<void(cc::websocket::connection&,const std::string&)> close_handler_;
    std::function<void(cc::websocket::connection&)> error_handler_;
    std::function<bool(const cc::Req&)> accept_handler_;
  };
  /// Allows the user to assign parameters using functions.
  /// `rule.name("name").methods(HTTP::POST)`
  template <typename T>
  struct RuleParameterTraits {
    using self_t=T;
    WebSocketRule& websocket() {
      auto p=new WebSocketRule(static_cast<self_t*>(this)->rule_);
      static_cast<self_t*>(this)->rule_to_upgrade_.reset(p);
      return *p;
    }

    self_t& name(std::string name) noexcept {
      static_cast<self_t*>(this)->name_=std::move(name);
      return static_cast<self_t&>(*this);
    }

    self_t& methods(HTTP method) {
      static_cast<self_t*>(this)->methods_=1<<static_cast<int>(method);
      return static_cast<self_t&>(*this);
    }

    template <typename ... MethodArgs>
    self_t& methods(HTTP method,MethodArgs ... args_method) {
      methods(args_method...);
      static_cast<self_t*>(this)->methods_|=1<<static_cast<int>(method);
      return static_cast<self_t&>(*this);
    }

  };

  /// A rule that can change its parameters during runtime.
  class DynamicRule : public BaseRule,public RuleParameterTraits<DynamicRule> {
    public:
    DynamicRule(std::string rule) : BaseRule(std::move(rule)) {}
    void validate() override {
      if (!erased_handler_) {
        throw std::runtime_error(name_+(!name_.empty()?": ":"")+"no handler for url "+rule_);
      }
    }

    void handle(const Req& req,Res& res,const routing_params& params) override {
      erased_handler_(req,res,params);
    }

    template <typename Func>
    void operator()(Func f) {
#ifdef MSVC_WORKAROUND
      using function_t=utility::function_traits<decltype(&Func::operator())>;
#else
      using function_t=utility::function_traits<Func>;
#endif
      erased_handler_=wrap(std::move(f),spell::gen_seq<function_t::arity>());
    }

    // enable_if Arg1 == Req && Arg2 == Res
    // enable_if Arg1 == Req && Arg2 != resposne
    // enable_if Arg1 != Req
#ifdef MSVC_WORKAROUND
    template <typename Func,size_t ... Indices>
#else
    template <typename Func,unsigned ... Indices>
#endif
    std::function<void(const Req&,Res&,const routing_params&)>
      wrap(Func f,spell::seq<Indices...>) {
#ifdef MSVC_WORKAROUND
      using function_t=utility::function_traits<decltype(&Func::operator())>;
#else
      using function_t=utility::function_traits<Func>;
#endif
      if (!spell::is_parameter_tag_compatible(
        spell::get_parameter_tag_runtime(rule_.c_str()),
        spell::compute_parameter_tag_from_args_list<
        typename function_t::template arg<Indices>...>::value)) {
        throw std::runtime_error("route_dynamic: Handler type is mismatched with URL parameters: "+rule_);
      }
      auto ret=detail::routing_handler_call_helper::Wrapped<Func,typename function_t::template arg<Indices>...>();
      ret.template set_<
        typename function_t::template arg<Indices>...
      >(std::move(f));
      return ret;
    }

    template <typename Func>
    void operator()(std::string name,Func&& f) {
      name_=std::move(name);
      (*this).template operator()<Func>(std::forward(f));
    }
    private:
    std::function<void(const Req&,Res&,const routing_params&)> erased_handler_;

  };

  /// Default rule created when ROUTE is called.
  template <typename ... Args>
  class TaggedRule : public BaseRule,public RuleParameterTraits<TaggedRule<Args...>> {
    public:
    using self_t=TaggedRule<Args...>;

    TaggedRule(std::string rule)
      : BaseRule(std::move(rule)) {}

    void validate() override {
      if (!handler_) {
        throw std::runtime_error(name_+(!name_.empty()?": ":"")+"no handler for url "+rule_);
      }
    }

    template <typename Func>
    typename std::enable_if<spell::CallHelper<Func,spell::S<Args...>>::value,void>::type
      operator()(Func&& f) {
      static_assert(spell::CallHelper<Func,spell::S<Args...>>::value||
                    spell::CallHelper<Func,spell::S<cc::Req,Args...>>::value,
                    "Handler type is mismatched with URL parameters");
      static_assert(!std::is_same<void,decltype(f(std::declval<Args>()...))>::value,
                    "Handler function cannot have void return type; valid return types: string, int, cc::Res, cc::returnable");

      handler_=(
      [f=std::move(f)]
      (const Req&,Res&res,Args ... args){
        res=Res(f(args...));
        res.end();
      });
    }

    template <typename Func>
    typename std::enable_if<
      !spell::CallHelper<Func,spell::S<Args...>>::value &&
      spell::CallHelper<Func,spell::S<cc::Req,Args...>>::value,
      void>::type
      operator()(Func&& f) {
      static_assert(spell::CallHelper<Func,spell::S<Args...>>::value||
                    spell::CallHelper<Func,spell::S<cc::Req,Args...>>::value,
                    "Handler type is mismatched with URL parameters");
      static_assert(!std::is_same<void,decltype(f(std::declval<cc::Req>(),std::declval<Args>()...))>::value,
                    "Handler function cannot have void return type; valid return types: string, int, cc::Res, cc::returnable");

      handler_=(
      [f=std::move(f)]
      (const cc::Req&req,cc::Res&res,Args ... args){
        res=Res(f(req,args...));
        res.end();
      });
    }

    template <typename Func>
    typename std::enable_if<
      !spell::CallHelper<Func,spell::S<Args...>>::value&&
      !spell::CallHelper<Func,spell::S<cc::Req,Args...>>::value &&
      spell::CallHelper<Func,spell::S<cc::Res&,Args...>>::value,
      void>::type
      operator()(Func&& f) {
      static_assert(spell::CallHelper<Func,spell::S<Args...>>::value||
                    spell::CallHelper<Func,spell::S<cc::Res&,Args...>>::value
                    ,
                    "Handler type is mismatched with URL parameters");
      static_assert(std::is_same<void,decltype(f(std::declval<cc::Res&>(),std::declval<Args>()...))>::value,
                    "Handler function with Res argument should have void return type");
      handler_=(
      [f=std::move(f)]
      (const cc::Req&,cc::Res&res,Args ... args){
        f(res,args...);
      });
    }

    template <typename Func>
    typename std::enable_if<
      !spell::CallHelper<Func,spell::S<Args...>>::value&&
      !spell::CallHelper<Func,spell::S<cc::Req,Args...>>::value&&
      !spell::CallHelper<Func,spell::S<cc::Res&,Args...>>::value,
      void>::type
      operator()(Func&& f) {
      static_assert(spell::CallHelper<Func,spell::S<Args...>>::value||
                    spell::CallHelper<Func,spell::S<cc::Req,Args...>>::value||
                    spell::CallHelper<Func,spell::S<cc::Req,cc::Res&,Args...>>::value
                    ,
                    "Handler type is mismatched with URL parameters");
      static_assert(std::is_same<void,decltype(f(std::declval<cc::Req>(),std::declval<cc::Res&>(),std::declval<Args>()...))>::value,
                    "Handler function with Res argument should have void return type");

      handler_=std::move(f);
    }

    template <typename Func>
    void operator()(std::string name,Func&& f) {
      name_=std::move(name);
      (*this).template operator()<Func>(std::forward(f));
    }

    void handle(const Req& req,Res& res,const routing_params& params) override {
      detail::routing_handler_call_helper::call<
        detail::routing_handler_call_helper::call_params<
        decltype(handler_)>,
        0,0,0,0,
        spell::S<Args...>,
        spell::S<>
      >()(
        detail::routing_handler_call_helper::call_params<
        decltype(handler_)>
      {handler_,params,req,res}
      );
    }

    private:
    std::function<void(const cc::Req&,cc::Res&,Args...)> handler_;

  };

  const int RULE_SPECIAL_REDIRECT_SLASH=1;

  /// A search tree.
  class Trie {
    public:
    struct Node {
      unsigned rule_index{};
      std::array<unsigned,static_cast<int>(ParamType::MAX)> param_childrens{};
      std::unordered_map<std::string,unsigned> children;

      bool IsSimpleNode() const {
        return
          !rule_index&&
          std::all_of(
            std::begin(param_childrens),
            std::end(param_childrens),
            [](unsigned x) { return !x; });
      }
    };

    Trie(): nodes_(1) {}

    ///Check whether or not the trie is empty.
    bool is_empty() {
      return nodes_.size()>1;
    }

    private:
    void optimizeNode(Node* node) {
      for (auto x:node->param_childrens) {
        if (!x)
          continue;
        Node* child=&nodes_[x];
        optimizeNode(child);
      }
      if (node->children.empty())
        return;
      bool mergeWithChild=true;
      for (auto& kv:node->children) {
        Node* child=&nodes_[kv.second];
        if (!child->IsSimpleNode()) {
          mergeWithChild=false;
          break;
        }
      }
      if (mergeWithChild) {
        decltype(node->children) merged;
        for (auto& kv:node->children) {
          Node* child=&nodes_[kv.second];
          for (auto& child_kv:child->children) {
            merged[kv.first+child_kv.first]=child_kv.second;
          }
        }
        node->children=std::move(merged);
        optimizeNode(node);
      } else {
        for (auto& kv:node->children) {
          Node* child=&nodes_[kv.second];
          optimizeNode(child);
        }
      }
    }

    void optimize() {
      optimizeNode(head());
    }

    public:
    void validate() {
      if (!head()->IsSimpleNode())
        throw std::runtime_error("Internal error: Trie header should be simple!");
      optimize();
    }

    std::pair<unsigned,routing_params> find(const std::string& req_url,const Node* node=nullptr,unsigned pos=0,routing_params* params=nullptr) const {
      routing_params empty;
      if (params==nullptr)
        params=&empty;

      unsigned found{};
      routing_params match_params;

      if (node==nullptr)
        node=head();
      if (pos==req_url.size())
        return {node->rule_index, *params};

      auto update_found=[&found,&match_params](std::pair<unsigned,routing_params>& ret) {
        if (ret.first&&(!found||found>ret.first)) {
          found=ret.first;
          match_params=std::move(ret.second);
        }
      };

      if (node->param_childrens[static_cast<int>(ParamType::INT)]) {
        char c=req_url[pos];
        if ((c>='0'&&c<='9')||c=='+'||c=='-') {
          char* eptr;
          errno=0;
          long long int value=strtoll(req_url.data()+pos,&eptr,10);
          if (errno!=ERANGE&&eptr!=req_url.data()+pos) {
            params->int_params.push_back(value);
            auto ret=find(req_url,&nodes_[node->param_childrens[static_cast<int>(ParamType::INT)]],eptr-req_url.data(),params);
            update_found(ret);
            params->int_params.pop_back();
          }
        }
      }

      if (node->param_childrens[static_cast<int>(ParamType::UINT)]) {
        char c=req_url[pos];
        if ((c>='0'&&c<='9')||c=='+') {
          char* eptr;
          errno=0;
          unsigned long long int value=strtoull(req_url.data()+pos,&eptr,10);
          if (errno!=ERANGE&&eptr!=req_url.data()+pos) {
            params->uint_params.push_back(value);
            auto ret=find(req_url,&nodes_[node->param_childrens[static_cast<int>(ParamType::UINT)]],eptr-req_url.data(),params);
            update_found(ret);
            params->uint_params.pop_back();
          }
        }
      }

      if (node->param_childrens[static_cast<int>(ParamType::DOUBLE)]) {
        char c=req_url[pos];
        if ((c>='0'&&c<='9')||c=='+'||c=='-'||c=='.') {
          char* eptr;
          errno=0;
          double value=strtod(req_url.data()+pos,&eptr);
          if (errno!=ERANGE&&eptr!=req_url.data()+pos) {
            params->double_params.push_back(value);
            auto ret=find(req_url,&nodes_[node->param_childrens[static_cast<int>(ParamType::DOUBLE)]],eptr-req_url.data(),params);
            update_found(ret);
            params->double_params.pop_back();
          }
        }
      }

      if (node->param_childrens[static_cast<int>(ParamType::STRING)]) {
        size_t epos=pos;
        for (; epos<req_url.size(); ++epos) {
          if (req_url[epos]=='/')
            break;
        }

        if (epos!=pos) {
          params->string_params.push_back(req_url.substr(pos,epos-pos));
          auto ret=find(req_url,&nodes_[node->param_childrens[static_cast<int>(ParamType::STRING)]],epos,params);
          update_found(ret);
          params->string_params.pop_back();
        }
      }

      if (node->param_childrens[static_cast<int>(ParamType::PATH)]) {
        size_t epos=req_url.size();

        if (epos!=pos) {
          params->string_params.push_back(req_url.substr(pos,epos-pos));
          auto ret=find(req_url,&nodes_[node->param_childrens[static_cast<int>(ParamType::PATH)]],epos,params);
          update_found(ret);
          params->string_params.pop_back();
        }
      }

      for (auto& kv:node->children) {
        const std::string& fragment=kv.first;
        const Node* child=&nodes_[kv.second];

        if (req_url.compare(pos,fragment.size(),fragment)==0) {
          auto ret=find(req_url,child,pos+fragment.size(),params);
          update_found(ret);
        }
      }

      return {found, match_params};
    }

    void add(const std::string& url,unsigned rule_index) {
      unsigned idx{0};

      for (unsigned i=0; i<url.size(); ++i) {
        char c=url[i];
        if (c=='<') {
          static struct ParamTraits {
            ParamType type;
            std::string name;
          } paramTraits[]=
          {
              { ParamType::INT, "<int>" },
              { ParamType::UINT, "<uint>" },
              { ParamType::DOUBLE, "<float>" },
              { ParamType::DOUBLE, "<double>" },
              { ParamType::STRING, "<str>" },
              { ParamType::STRING, "<string>" },
              { ParamType::PATH, "<path>" },
          };

          for (auto& x:paramTraits) {
            if (url.compare(i,x.name.size(),x.name)==0) {
              if (!nodes_[idx].param_childrens[static_cast<int>(x.type)]) {
                auto new_node_idx=new_node();
                nodes_[idx].param_childrens[static_cast<int>(x.type)]=new_node_idx;
              }
              idx=nodes_[idx].param_childrens[static_cast<int>(x.type)];
              i+=x.name.size();
              break;
            }
          }
          --i;
        } else {
          std::string piece(&c,1);
          if (!nodes_[idx].children.count(piece)) {
            auto new_node_idx=new_node();
            nodes_[idx].children.emplace(piece,new_node_idx);
          }
          idx=nodes_[idx].children[piece];
        }
      }
      if (nodes_[idx].rule_index)
        throw std::runtime_error("handler already exists for "+url);
      nodes_[idx].rule_index=rule_index;
    }
    private:
    void debug_node_print(Node* n,int level) {
      for (int i=0; i<static_cast<int>(ParamType::MAX); ++i) {
        if (n->param_childrens[i]) {
          LOG_DEBUG<<std::string(2*level,' ') /*<< "("<<n->param_childrens[i]<<") "*/;
          switch (static_cast<ParamType>(i)) {
            case ParamType::INT:
            LOG_DEBUG<<"<int>";
            break;
            case ParamType::UINT:
            LOG_DEBUG<<"<uint>";
            break;
            case ParamType::DOUBLE:
            LOG_DEBUG<<"<float>";
            break;
            case ParamType::STRING:
            LOG_DEBUG<<"<str>";
            break;
            case ParamType::PATH:
            LOG_DEBUG<<"<path>";
            break;
            default:
            LOG_DEBUG<<"<ERROR>";
            break;
          }

          debug_node_print(&nodes_[n->param_childrens[i]],level+1);
        }
      }
      for (auto& kv:n->children) {
        LOG_DEBUG<<std::string(2*level,' ') /*<< "(" << kv.second << ") "*/<<kv.first;
        debug_node_print(&nodes_[kv.second],level+1);
      }
    }

    public:
    void debug_print() {
      debug_node_print(head(),0);
    }

    private:
    const Node* head() const {
      return &nodes_.front();
    }

    Node* head() {
      return &nodes_.front();
    }

    unsigned new_node() {
      nodes_.resize(nodes_.size()+1);
      return nodes_.size()-1;
    }

    std::vector<Node> nodes_;
  };


  /// Handles matching requests to existing rules and upgrade requests.

  class Router {
    public: Router() {}
    DynamicRule& new_rule_dynamic(const std::string& rule) {
      auto ruleObject=new DynamicRule(rule);
      all_rules_.emplace_back(ruleObject);
      return *ruleObject;
    }
    template <uint64_t N>
    typename spell::arguments<N>::type::template rebind<TaggedRule>& new_rule_tagged(const std::string& rule) {
      using RuleT=typename spell::arguments<N>::type::template rebind<TaggedRule>;
      auto ruleObject=new RuleT(rule);
      all_rules_.emplace_back(ruleObject);
      return *ruleObject;
    }
    CatchallRule& catchall_rule() {
      return catchall_rule_;
    }
    void internal_add_rule_object(const std::string& rule,BaseRule* ruleObject) {
      bool has_trailing_slash=false;
      std::string rule_without_trailing_slash;
      if (rule.size()>1&&rule.back()=='/') {
        has_trailing_slash=true;
        rule_without_trailing_slash=rule;
        rule_without_trailing_slash.pop_back();
      }
      ruleObject->foreach_method([&](int method) {
        per_methods_[method].rules.emplace_back(ruleObject);
        per_methods_[method].trie.add(rule,per_methods_[method].rules.size()-1);
        // directory case: 
        //   request to `/about' url matches `/about/' rule 
        if (has_trailing_slash) {
          per_methods_[method].trie.add(rule_without_trailing_slash,RULE_SPECIAL_REDIRECT_SLASH);
        }
      });
    }
    void validate() {
      for (auto& rule:all_rules_) {
        if (rule) {
          auto upgraded=rule->upgrade();
          if (upgraded)
            rule=std::move(upgraded);
          rule->validate();
          internal_add_rule_object(rule->rule(),rule.get());
        }
      }
      for (auto& per_method:per_methods_) {
        per_method.trie.validate();
      }
    }
    template <typename Adaptor>
    void handle_upgrade(const Req& req,Res& res,Adaptor&& adaptor) {
      if (req.method>=HTTP::InternalMethodCount)
        return;

      auto& per_method=per_methods_[static_cast<int>(req.method)];
      auto& rules=per_method.rules;
      unsigned rule_index=per_method.trie.find(req.url).first;

      if (!rule_index) {
        for (auto& per_method:per_methods_) {
          if (per_method.trie.find(req.url).first) {
            LOG_DEBUG<<"Cannot match method "<<req.url<<" "<<m2s(req.method);
            res=Res(405);
            res.end();
            return;
          }
        }

        LOG_INFO<<"913:Cannot match rules "<<req.url;
        res=Res(404);
        res.end();
        return;
      }

      if (rule_index>=rules.size())
        throw std::runtime_error("Trie internal structure corrupted!");

      if (rule_index==RULE_SPECIAL_REDIRECT_SLASH) {
        LOG_INFO<<"Redirecting to a url with trailing slash: "<<req.url;
        res=Res(301);

        // TODO absolute url building
        if (req.get_header_value("Host").empty()) {
          res.add_header("Location",req.url+"/");
        } else {
          res.add_header("Location","http://"+req.get_header_value("Host")+req.url+"/");
        }
        res.end();
        return;
      }

      LOG_DEBUG<<"936:Matched rule (upgrade) '"<<rules[rule_index]->rule_<<"' "<<static_cast<uint32_t>(req.method)<<" / "<<rules[rule_index]->get_methods();

      // any uncaught exceptions become 500s
      try {
        rules[rule_index]->handle_upgrade(req,res,std::move(adaptor));
      } catch (std::exception& e) {
        LOG_ERROR<<"An uncaught exception occurred: "<<e.what();
        res=Res(500);
        res.end();
        return;
      } catch (...) {
        LOG_ERROR<<"An uncaught exception occurred. The type was unknown so no information was available.";
        res=Res(500);
        res.end();
        return;
      }
    }

    void handle(const Req& req,Res& res) {
      HTTP method_actual=req.method;
      if (method_actual>=HTTP::InternalMethodCount)
        return;
      auto& per_method=per_methods_[static_cast<int>(method_actual)];
      if (req.method==HTTP::HEAD) {
        method_actual=HTTP::GET;
        res.is_head_response=true;
      } else if (method_actual==HTTP::OPTIONS) {
        std::string allow="OPTIONS, HEAD, ";
        if (req.url=="/*") {
          for (int i=0; i<static_cast<int>(HTTP::InternalMethodCount); ++i) {
            if (per_methods_[i].trie.is_empty()) {
              allow+=m2s(static_cast<HTTP>(i))+", ";
            }
          }
          allow=allow.substr(0,allow.size()-2);
          res=Res(204);
          res.set_header("Allow",allow);
          res.end();
          return;
        } else {
          for (int i=0; i<static_cast<int>(HTTP::InternalMethodCount); ++i) {
            if (per_methods_[i].trie.find(req.url).first) {
              allow+=m2s(static_cast<HTTP>(i))+", ";
            }
          }
          if (allow!="OPTIONS, HEAD, ") {
            allow=allow.substr(0,allow.size()-2);
            res=Res(204);
            res.set_header("Allow",allow);
            res.end();
            return;
          } else {
            LOG_DEBUG<<"988:Cannot match rules "<<req.url;
            res=Res(404);
            res.end();
            return;
          }
        }
      }
      auto& trie=per_method.trie;
      auto& rules=per_method.rules;
      auto found=trie.find(req.url);
      unsigned rule_index=found.first;
      /*if (catchall_rule_.has_handler()) {
        LOG_DEBUG<<"1010:Cannot match rules "<<req.url<<". Redirecting to Catchall rule";
        catchall_rule_.handler_(req,res);std::cout<<res.body;return;
      }*/
      if (!rule_index) {
        for (auto& per_method:per_methods_) {
          if (per_method.trie.find(req.url).first) {
            LOG_DEBUG<<"Cannot match method "<<req.url<<" "<<m2s(method_actual);
            res=Res(405);
            res.end();
            return;
          }
        }
        res.set_static_file_info(req.url.substr(1));
        if (res.code==404&&catchall_rule_.has_handler()) {
          LOG_DEBUG<<"1010:Cannot match rules "<<req.url<<". Redirecting to Catchall rule";
          res.code=200;catchall_rule_.handler_(req,res);
        } else res.end();
        return;
      }
      if (rule_index>=rules.size())
        throw std::runtime_error("Trie internal structure corrupted!");
      if (rule_index==RULE_SPECIAL_REDIRECT_SLASH) {
        LOG_INFO<<"Redirecting to a url with trailing slash: "<<req.url;
        res=Res(301);
        // TODO absolute url building
        if (req.get_header_value("Host").empty()) {
          res.add_header(RES_Loc,req.url+"/");
        } else {
          res.add_header(RES_Loc,"http://"+req.get_header_value("Host")+req.url+"/");
        }
        res.end();
        return;
      }
      LOG_DEBUG<<"1027:Matched rule '"<<rules[rule_index]->rule_<<"' "<<static_cast<uint32_t>(req.method)<<" / "<<rules[rule_index]->get_methods();
      // any uncaught exceptions become 500s
      try {
        rules[rule_index]->handle(req, res, found.second);
      } catch (std::exception& e) {
        res.code = 500;
        res.body = e.what();//An uncaught exception occurred:
        LOG_ERROR << e.what();
        res.end();
        return;
      } catch (...) {
        LOG_ERROR << "An uncaught exception occurred. The type was unknown so no information was available.";
        res.code = 500;
        res.body = "500 Internal Server Error";
        res.end();
        return;
      }
    }

    void debug_print() {
      for (int i=0; i<static_cast<int>(HTTP::InternalMethodCount); ++i) {
        LOG_DEBUG<<m2s(static_cast<HTTP>(i));
        per_methods_[i].trie.debug_print();
      }
    }
    private:
    CatchallRule catchall_rule_;
    struct PerMethod {
      std::vector<BaseRule*> rules;
      Trie trie;
      // rule index 0, 1 has special meaning; preallocate it to avoid duplication.
      PerMethod(): rules(2) {}
    };
    std::array<PerMethod,static_cast<int>(HTTP::InternalMethodCount)> per_methods_;
    std::vector<std::unique_ptr<BaseRule>> all_rules_;
  };
}