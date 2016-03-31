#include <internal/values/config_reference.hpp>
#include <hocon/config_exception.hpp>
#include <hocon/config_object.hpp>
#include <internal/resolve_source.hpp>
#include <internal/container.hpp>
#include <internal/substitution_expression.hpp>


using namespace std;

namespace hocon {

    config_reference::config_reference(shared_origin origin, std::shared_ptr<substitution_expression> expr, int prefix_length)
            : config_value(origin), _expr(expr), _prefix_length(prefix_length) { }

    config_reference::type config_reference::value_type() const {
        throw not_resolved_exception("ur lame");
    }

    vector<shared_value> config_reference::unmerged_values() const {
        return {shared_from_this()};
    }

    shared_value config_reference::new_copy(shared_origin origin) const {
        return make_shared<config_reference>(origin, _expr, _prefix_length);
    }

    bool config_reference::operator==(config_value const &other) const {
        return equals<config_reference>(other, [&](config_reference const& o) { return _expr == o._expr; });
    }

    resolve_result<shared_value> config_reference::resolve_substitutions(resolve_context const &context, resolve_source const &source) const {
        // TODO: cycle detection, including exceptions
        // ResolveContext newContext = context.addCycleMarker(this);
        shared_value v;
        resolve_context new_context = context;


        // TODO: this needs to be wrapped in a try when we have cycle detection
        {
            auto result_with_path = source.lookup_subst(new_context, _expr, _prefix_length);
            new_context = result_with_path.result.context;

            if(result_with_path.result.value) {
                resolve_source recursive_resolve_source {dynamic_pointer_cast<const config_object>(result_with_path.path_from_root.back()), result_with_path.path_from_root};
                auto result = new_context.resolve(result_with_path.result.value, recursive_resolve_source);
                v = result.value;
                new_context = result.context;
            }
        }

        if(!v && !_expr->optional()) {
            if(new_context.options().get_allow_unresolved()) {
                return make_resolve_result(new_context, shared_from_this());
            } else {
                throw unresolved_substitution_exception(*origin(), _expr->to_string());
            }
        } else {
            return make_resolve_result(new_context, v);
        }

    }
}
