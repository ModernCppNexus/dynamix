// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include "throw_exception.hpp"
#include "domain.hpp"
#include "feature_info.hpp"
#include "mixin_info.hpp"
#include "type_mutation.hpp"
#include "exception.hpp"
#include "error_return.hpp"
#include "type.hpp"

#include <sstream>

namespace dynamix::throw_exception {
namespace {
template <typename Exception>
class e {
    std::ostringstream out;
public:
    e(const domain& dom) {
        *this << dom << ": ";
    }

    e& operator<<(const domain& dom) {
        if (dom.name().empty()) {
            out << "unnamed domain";
        }
        else {
            out << dom.name();
        }
        return *this;
    }

    e& operator<<(const feature_info& info) {
        out << '\'' << info.name.to_std() << '\'';
        return *this;
    }

    e& operator<<(const mixin_info& info) {
        out << '\'' << info.name.to_std() << '\'';
        return *this;
    }

    e& operator<< (const mutation_rule_info& info) {
        out << '\'' << info.name.to_std() << '\'';
        return *this;
    }

    void output_mixins(const itlib::span<const mixin_info* const>& mixins) {
        out << '{';
        bool first = true;
        for (const auto& m : mixins) {
            if (first) {
                first = false;
            }
            else {
                out << ", ";
            }
            *this << *m;
        }
        out << '}';
    }

    e& operator<<(const type_mutation& mut) {
        output_mixins(mut.mixins);
        return *this;
    }

    e& operator<<(const type& t) {
        output_mixins(t.mixins);
        return *this;
    }

    template <typename T>
    e& operator<<(const T& t) {
        out << t;
        return *this;
    }

    [[noreturn]] ~e() noexcept(false) {
        throw Exception(out.str());
    }
};
}

void id_registered(const domain& dom, const feature_info& info) {
    e<domain_error>(dom) << "register feature " << info << " with a valid id " << info.iid();
}

void id_registered(const domain& dom, const mixin_info& info) {
    e<domain_error>(dom) << "register mixin " << info << " with a valid id " << info.iid();
}

void empty_name(const domain& dom, const feature_info&) {
    e<domain_error>(dom) << "register feature with empty name";
}

void empty_name(const domain& dom, const mixin_info&) {
    e<domain_error>(dom) << "register mixin with empty name";
}

void empty_name(const domain& dom, const type_class&) {
    e<domain_error>(dom) << "register type class with empty name";
}

void duplicate_name(const domain& dom, const feature_info& info) {
    e<domain_error>(dom) << "register feature with duplicate name '" << info.name.to_std() << '\'';
}

void duplicate_name(const domain& dom, const mixin_info& info) {
    e<domain_error>(dom) << "register mixin with duplicate name '" << info.name.to_std() << '\'';
}

void duplicate_name(const domain& dom, const type_class& tc) {
    e<domain_error>(dom) << "register type class with duplicate name '" << tc.name.to_std() << '\'';
}

void info_has_domain(const domain& dom, const mixin_info& info) {
    e<domain_error>(dom) << "register mixin " << info << " which has a domain = " << *domain::from_c_handle(info.dom);;
}

void unreg_foreign(const domain& dom, const feature_info& info) {
    e<domain_error>(dom) << "unregister foreign feature " << info << ", id = " << info.iid();
}
void unreg_foreign(const domain& dom, const mixin_info& info) {
    e<domain_error> out(dom);
    out << "unregister foreign feature " << info << ", id = " << info.iid() << ", domain = ";
    if (info.dom) {
        out << *domain::from_c_handle(info.dom);
    }
    else {
        out << "<null>";
    }
}

void no_func(const domain& dom, const mutation_rule_info& info) {
    e<domain_error>(dom) << "register mutation rule " << info << " with apply function = null";
}

void no_func(const domain& dom, const type_class& tc) {
    e<domain_error>(dom) << "register type class '" << tc.name.to_std() << "' with match function = null";
}

void mutation_rule_user_error(const type_mutation& mut, const mutation_rule_info& info, error_return_t error) {
    e<mutation_error>(mut.dom) << "applying mutation rule " << info << " to " << mut << " failed with error " << error;
}

void foreign_domain(const domain& dom, const type_mutation& mut) {
    e<mutation_error>(dom) << "requested type with foreign mutation " << mut << " of domain '" << mut.dom << '\'';
}

void cyclic_rule_deps(const type_mutation& mut) {
    e<domain_error>(mut.dom) << "rule interdependency too deep or cyclic at " << mut;
}

void mut_unreg_mixin(const type_mutation& mut, const mixin_info& m) {
    e<mutation_error>(mut.dom) << "unregistered mixin " << m << " while trying to create type " << mut;
}

void mut_foreign_mixin(const type_mutation& mut, const mixin_info& m) {
    e<mutation_error> out(mut.dom);
    out << "foreign mixin " << m << " from ";
    if (m.dom) {
        out << '\'' << *domain::from_c_handle(m.dom) << '\'';
    }
    else {
        out << "<null>";
    }
    out << " while trying to create type " << mut;
}

void mut_dup_mixin(const type_mutation& mut, const mixin_info& m) {
    e<mutation_error>(mut.dom) << "duplicate mixin " << m << " while trying to create type " << mut;
}

void feature_clash(const type_mutation& mut, const dnmx_ftable_payload& a, const dnmx_ftable_payload& b) {
    e<mutation_error>(mut.dom) << "feature clash in " << mut << " on " << *a.data->info << " between " <<
        *mut.mixins[a.mixin_index] << " and " << *mut.mixins[b.mixin_index];
}

void obj_mut_error(const type& t, std::string_view op, std::string_view err, const mixin_info& m) {
    e<mutation_error>(t.dom) << op << " on object of type " << t << ": " << m << ' ' << err;
}

void obj_mut_user_error(const type& t, std::string_view op, std::string_view ovr, const mixin_info& m, error_return_t error) {
    e<mutation_error>(t.dom) << op << " on object of type " << t << ": " << ovr << ' ' << m << " failed with error " << error;
}

void obj_mut_sealed_object(const type& t, std::string_view op) {
    e<mutation_error>(t.dom) << op << " on sealed object of type " << t;
}

void obj_error(const type& t, std::string_view op) {
    e<mutation_error>(t.dom) << op << " on object of type " << t;
}

}
