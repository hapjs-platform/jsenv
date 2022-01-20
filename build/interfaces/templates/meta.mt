// Copyright (c) 2021, the hapjs-platform Project Contributors
// SPDX-License-Identifier: EPL-1.0

#define ${interface.implement_guard} 1

#include "${interface.header}"

#include "base/interfaces/interface.h"
#include "base/interfaces/interface_utils.h"

using base::interface_base;
using base::interface_values_reader;
using base::interface_values_reader_holder;
using base::error_message;
using base::interface_event_handler_base;
using base::interface_meta;
using base::member_meta;

%for namespace in interface.namespaces:
namespace ${namespace} {
%endfor

class Interface${interface.name} : public interface_base<${interface.name}> {
 public:
  using interface_type = ${interface.name};
  using interface_ref = base::ref<interface_type>;
  using event_handler = interface_event_handler_base<interface_type>;

  const interface_meta* get_meta() const override {
    return &_meta;
  }

  interface_ref create(const interface_values_reader* pargs,
                       error_message& error) const override;

  int call_method(interface_type* self, int method_index,
                  const interface_values_reader* args,
                  interface_values_reader_holder* presult,
                  error_message& error) const override;

  int get_property(interface_type* self, int prop_index,
                   interface_values_reader_holder* presult,
                   error_message& error) const override;

  int set_property(interface_type* self, int prop_index,
                   const interface_values_reader* args,
                   error_message& error) const override;

  int set_event(interface_type* self, base::ref<event_handler> handle) const override;

  base::ref<event_handler> get_unique_handle(interface_type* self, int key) const override;

 private:
  static base::interface_meta _meta;
}; // Interface${interface.name}

const base::interface* ${interface.name}_get_interface() {
  static Interface${interface.name} _interface;
  return reinterpret_cast<const base::interface*>(&_interface);
}

static member_meta _${interface.name}_members [] = {
%for method in interface.methods:
  {
    base::kInterfaceMethod,
    0,
    ${interface.name}::${method.index_name},
    "${method.name.fullname}",
    "${method.signature}"
  },
%endfor
%for prop in interface.properties:
  {
    base::kInterfaceProperty,
%if prop.readonly:
    base::kInterfacePropertyReadable,
%else:
    base::kInterfacePropertyReadable | base::kInterfacePropertyWritable,
%endif
    ${interface.name}::${prop.index_name},
    "${prop.name.fullname}",
    "${prop.signature}"
  },
%endfor
};

base::interface_meta Interface${interface.name}::_meta = {
  base::kInterface,
%if interface.HasEvents():
  base::kInterfaceHasEvents,
%else:
  0,
%endif
  MEMBER_COUNT(_${interface.name}_members),
  "${interface.fullname}",
  nullptr, // use extends
  _${interface.name}_members
};

///////////////////////////////////////////////////////
// implements

base::ref<${interface.name}> Interface${interface.name}::create(
          const interface_values_reader* pargs,
          error_message& error) const {
%if len(interface.constructors) > 0:
<% ctr_idx = 0 %>
%for ctr in interface.constructors:
<%
  ctr_idx = ctr_idx + 1
  ctr_arg_types = ['interface_values_reader::%s'%type for type in ctr.argument_types]
  ctr_arg_call = ['base::get<interface_values_reader::%s>(pargs, %d)' % (ctr.argument_types[i], i) \
          for i in range(0, len(ctr.argument_types))]
%>
  do {
    static int types[] = {${', '.join(ctr_arg_types)}};
    if (base::check_match(pargs, types, sizeof(types)/sizeof(types[0]))) {
      return ${interface.name}::Create${ctr.name.fullname}(${', '.join(ctr_arg_call)});
    }
  } while(0);
%endfor
  if (pargs->get_count() == 0) {
    return ${interface.name}::Create${interface.name}();
  }
%endif
  error.set(base::kInterfaceInternalError, "unkown how to create object");
  return base::ref<${interface.name}>();
}

int Interface${interface.name}::call_method(${interface.name}* self, int method_index,
              const interface_values_reader* pargs,
              interface_values_reader_holder* presult,
              error_message& error) const {
  switch(method_index) {
%for method in interface.methods:
  case ${interface.name}::${method.index_name}:
<%
  method_arg_call = ['base::get<interface_values_reader::%s>(pargs, %d)' % (method.call_types[i], i - 1) \
        for i in range(1, len(method.call_types))]
%>
%if method.call_types[0] != 'kVoid':
    base::make_reader(presult,
%endif
      self->${method.name.fullname}(
          ${', '.join(method_arg_call)}
%if method.call_types[0] != 'kVoid':
      ));
%else:
      );
%endif
    break;
%endfor
  default:
    base::make_reader(presult, 0); // to avoid unused arguments error
    error.set(base::kInterfaceMethodNotImplement, "method is not support");
    return base::kInterfaceMethodNotImplement;
  }
  return base::kInterfaceOk;
}

int Interface${interface.name}::get_property(${interface.name}* self, int prop_index,
              interface_values_reader_holder* presult,
              error_message& error) const {
  switch(prop_index) {
%for prop in interface.properties:
  case ${interface.name}::${prop.index_name}:
    make_reader(presult, self->get_${prop.name.fullname}());
    break;
%endfor
  default:
    base::make_reader(presult, 0); // to avoid unused arguments error
    error.set(base::kInterfacePropertyNotImplement, "property is not support");
    return base::kInterfacePropertyNotImplement;
  }
  return base::kInterfaceOk;
}

int Interface${interface.name}::set_property(${interface.name}* self, int prop_index,
              const interface_values_reader* pargs,
              error_message& error) const {
  switch(prop_index) {
%for prop in interface.properties:
  case ${interface.name}::${prop.index_name}:
  %if prop.readonly:
    error.set(base::kInterfacePropertyReadOnly, "property is readonly");
    return base::kInterfacePropertyReadOnly;
  %else:
    self->set_${prop.name.fullname}(base::get<interface_values_reader::${prop.value_type}>(pargs, 0));
    break;
  %endif
%endfor
  default:
    error.set(base::kInterfacePropertyNotImplement, "property is not support");
    return base::kInterfacePropertyNotImplement;
  }
  return base::kInterfaceOk;
}

int Interface${interface.name}::set_event(interface_type* self,
        base::ref<interface_event_handler_base<${interface.name}>> handle) const {
  return self->add_event_handler(handle);
}

base::ref<interface_event_handler_base<${interface.name}>>
  Interface${interface.name}::get_unique_handle(${interface.name}* self, int key) const {
  return self->get_unique_event_handle(key);
}

%for i in range(len(interface.namespaces), 0, -1):
}  // namespace ${interface.namespaces[i-1]}
%endfor
