// Copyright (c) 2021, the hapjs-platform Project Contributors
// SPDX-License-Identifier: EPL-1.0

#ifndef ${interface.header_guard}
#define ${interface.header_guard}

#include "base/interfaces/interface.h"
#include "base/interfaces/interface_manager.h"
#include "base/interfaces/interface_utils.h"

%for imp in interface.import_headers:
#include "${imp}"
%endfor

%for namespace in interface.namespaces:
namespace ${namespace} {
%endfor

class ${interface.name} : public base::ref_base<${interface.name}> {
 public:
  virtual ~${interface.name}() {}

  inline base::ref<${interface.name}> ref_from() {
    return base::as<${interface.name}>(base::ref_base<${interface.name}>::ref_from());
  }

  static inline const char* get_interface_fullname() {
    return "{interface.fullname}";
  }

  // define the method index
  enum {
%for method in interface.methods:
    ${method.index_name},
%endfor
  };

  // define the property index
  enum {
%for prop in interface.properties:
    ${prop.index_name},
%endfor
  };

  // define the methods
%for method in interface.methods:
<%
  cpp_arg_defines = ['%s %s' % (arg.type, arg.name) for arg in method.cpp_define[1:]]
%>
  virtual ${method.cpp_define[0].type} ${method.name.fullname}(${', '.join(cpp_arg_defines)}) = 0;
%endfor


  // define the property
%for prop in interface.properties:
  virtual ${prop.cpp_type.type} get_${prop.name.fullname}() const = 0;
%if not prop.readonly:
  virtual void set_${prop.name.fullname}(${prop.cpp_type.type} val) = 0;
%endif

%endfor

  // define the events
%for event in interface.events:
<%
  event_args = [event.final and 'true' or 'false'] + \
               [arg.type for arg in event.cpp_define]
  event_name = 'base::event<%s>' % ','.join(event_args);
%>
  using ${event.name.fullname}_type = ${event_name};
  static inline const ${event.name.fullname}_type& ${event.name.fullname}() {
    static ${event.name.fullname}_type evt("${event.name.fullname}");
    return evt;
  }
%endfor

  virtual int add_event_handler(base::ref<base::interface_event_handler_base<${interface.name}>> handle) {
    return 0;
  }

  virtual void add_handler(base::ref<base::handler<${interface.name}>> handle) {
  }

  virtual base::ref<base::interface_event_handler_base<${interface.name}>>
    get_unique_event_handle(int key) {
      return base::ref<base::interface_event_handler_base<${interface.name}>>();
  }

 public:
#if ${interface.implement_guard}
  // must implement by user
%for ctr in interface.constructors:
<%
  cpp_arg_defines = ['%s %s'% (arg.type, arg.name) for arg in ctr.cpp_define]
%>
  static base::ref<${interface.name}> Create${ctr.name.fullname}(${', '.join(cpp_arg_defines)});
%endfor

  // default creator
  static base::ref<${interface.name}> Create${interface.name}();
#else
  static base::interface_auto_creator<${interface.name}>& get_creator() {
    static base::interface_auto_creator<${interface.name}> creator;
    return creator;
  }
  // call by user
%for ctr in interface.constructors:
<%
  cpp_arg_defines = ['%s %s'% (arg.type, arg.name) for arg in ctr.cpp_define]
  cpp_call_args = [arg.name for arg in ctr.cpp_define]
%>
  static base::ref<${interface.name}> Create${ctr.name.fullname}(${', '.join(cpp_arg_defines)}) {
    return get_creator()(${', '.join(cpp_call_args)});
  }
%endfor

  // default creator
  static base::ref<${interface.name}> Create${interface.name}() {
    return get_creator()();
  }
#endif

};


%for i in range(len(interface.namespaces), 0, -1):
}  // namespace ${interface.namespaces[i-1]}
%endfor
#endif  // ${interface.header_guard}
