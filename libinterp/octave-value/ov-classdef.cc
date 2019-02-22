/*

Copyright (C) 2012-2019 Michael Goffioul

This file is part of Octave.

Octave is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Octave is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Octave; see the file COPYING.  If not, see
<https://www.gnu.org/licenses/>.

*/

#if defined (HAVE_CONFIG_H)
#  include "config.h"
#endif

#include <algorithm>
#include <iomanip>

#include "call-stack.h"
#include "cdef-utils.h"
#include "defun.h"
#include "errwarn.h"
#include "interpreter-private.h"
#include "load-path.h"
#include "ov-classdef.h"
#include "ov-fcn-handle.h"
#include "ov-typeinfo.h"
#include "ov-usr-fcn.h"
#include "parse.h"
#include "pt-eval.h"
#include "pt-misc.h"
#include "symtab.h"

static bool
in_class_method (const cdef_class& cls)
{
  cdef_class ctx = get_class_context ();

  return (ctx.ok () && is_superclass (ctx, cls));
}

//----------------------------------------------------------------------------

int octave_classdef::t_id (-1);

const std::string octave_classdef::t_name ("object");

void
octave_classdef::register_type (octave::type_info& ti)
{
  t_id = ti.register_type (octave_classdef::t_name, "<unknown>",
                           octave_value (new octave_classdef ()));
}

octave_value_list
octave_classdef::subsref (const std::string& type,
                          const std::list<octave_value_list>& idx,
                          int nargout)
{
  size_t skip = 0;
  octave_value_list retval;

  cdef_class cls = object.get_class ();

  if (! in_class_method (cls) && ! called_from_builtin ())
    {
      cdef_method meth = cls.find_method ("subsref");

      if (meth.ok ())
        {
          octave_value_list args;

          args(1) = make_idx_args (type, idx, "subsref");

          count++;
          args(0) = octave_value (this);

          retval = meth.execute (args, nargout, true, "subsref");

          return retval;
        }
    }

  // At this point, the default subsref mechanism must be used.

  retval = object.subsref (type, idx, nargout, skip, cdef_class ());

  if (type.length () > skip && idx.size () > skip)
    retval = retval(0).next_subsref (nargout, type, idx, skip);

  return retval;
}

octave_value
octave_classdef::subsref (const std::string& type,
                          const std::list<octave_value_list>& idx,
                          bool auto_add)
{
  size_t skip = 0;
  octave_value_list retval;

  // This variant of subsref is used to create temporary values when doing
  // assignment with multi-level indexing.  AFAIK this is only used for internal
  // purpose (not sure we should even implement this).

  cdef_class cls = object.get_class ();

  if (! in_class_method (cls))
    {
      cdef_method meth = cls.find_method ("subsref");

      if (meth.ok ())
        {
          octave_value_list args;

          args(1) = make_idx_args (type, idx, "subsref");

          count++;
          args(0) = octave_value (this);

          retval = meth.execute (args, 1, true, "subsref");

          return retval.length () > 0 ? retval(0) : octave_value ();
        }
    }

  retval = object.subsref (type, idx, 1, skip, cdef_class (), auto_add);

  if (type.length () > skip && idx.size () > skip)
    retval = retval(0).next_subsref (1, type, idx, skip);

  return retval.length () > 0 ? retval(0) : octave_value ();
}

octave_value
octave_classdef::subsasgn (const std::string& type,
                           const std::list<octave_value_list>& idx,
                           const octave_value& rhs)
{
  octave_value retval;

  cdef_class cls = object.get_class ();

  if (! in_class_method (cls) && ! called_from_builtin ())
    {
      cdef_method meth = cls.find_method ("subsasgn");

      if (meth.ok ())
        {
          octave_value_list args;

          args(1) = make_idx_args (type, idx, "subsasgn");

          count++;
          args(0) = octave_value (this);
          args(2) = rhs;

          octave_value_list retlist;

          retlist = meth.execute (args, 1, true, "subsasgn");

          if (retlist.empty ())
            error ("overloaded method `subsasgn' did not return any value");

          retval = retlist(0);
        }
    }

  if (! retval.is_defined ())
    retval = object.subsasgn (type, idx, rhs);

  return retval;
}

octave_value
octave_classdef::undef_subsasgn (const std::string& type,
                                 const std::list<octave_value_list>& idx,
                                 const octave_value& rhs)
{
  if (type.length () == 1 && type[0] == '(')
    {
      object = object.make_array ();

      return subsasgn (type, idx, rhs);
    }
  else
    return octave_base_value::undef_subsasgn (type, idx, rhs);

  return octave_value ();
}

octave_idx_type
octave_classdef::numel (const octave_value_list& idx)
{
  octave_idx_type retval = -1;

  cdef_class cls = object.get_class ();

  if (! in_class_method (cls) && ! called_from_builtin ())
    {
      cdef_method meth = cls.find_method ("numel");

      if (meth.ok ())
        {
          octave_value_list args (idx.length () + 1, octave_value ());

          count++;
          args(0) = octave_value (this);

          for (octave_idx_type i = 0; i < idx.length (); i++)
            args(i+1) = idx(i);

          octave_value_list lv = meth.execute (args, 1, true, "numel");
          if (lv.length () != 1 || ! lv(0).is_scalar_type ())
            error ("@%s/numel: invalid return value", cls.get_name ().c_str ());

          retval = lv(0).idx_type_value (true);

          return retval;
        }
    }

  retval = octave_base_value::numel (idx);

  return retval;
}

void
octave_classdef::print (std::ostream& os, bool)
{
  print_raw (os);
}

void
octave_classdef::print_raw (std::ostream& os, bool) const
{
  cdef_class cls = object.get_class ();

  if (cls.ok ())
    {
      indent (os);
      os << class_name () << " object";
      if (object.is_array ())
        os << " array";
      os << " with properties:";
      newline (os);
      newline (os);

      std::map<std::string, cdef_property> props;

      props = cls.get_property_map ();

      size_t max_len = 0;
      for (const auto& nm_prop : props)
        {
          const std::string& nm = nm_prop.first;

          size_t sz = nm.size ();

          if (sz > max_len)
            max_len = sz;
        }

      for (auto& nm_prop : props)
        {
          const std::string& nm = nm_prop.first;
          cdef_property& prop = nm_prop.second;
          octave_value val = prop.get_value (object, false);
          dim_vector dims = val.dims ();

          os << std::setw (max_len+2) << nm << ": ";
          if (val.is_string ())
            os << val.string_value ();
          else if (val.islogical ())
            os << val.bool_value ();
          else
            os << "[" << dims.str () << " " << val.class_name () << "]";
          newline (os);
        }
    }
}

bool
octave_classdef::is_instance_of (const std::string& cls_name) const
{
  cdef_class cls = lookup_class (cls_name, false, false);

  if (cls.ok ())
    return is_superclass (cls, object.get_class ());

  return false;
}

//----------------------------------------------------------------------------

octave_value
octave_classdef::superclass_ref (const std::string& meth,
                                 const std::string& cls)
{
  return octave_value (new octave_classdef_superclass_ref (meth, cls));
}

octave_value
octave_classdef::metaclass_query (const std::string& cls)
{
  return to_ov (lookup_class (cls));
}

//----------------------------------------------------------------------------

bool octave_classdef_meta::is_classdef_constructor (const std::string& cname) const
{
  bool retval = false;

  if (object.is_class ())
    {
      if (cname.empty ())
        retval = true;
      else
        {
          cdef_class cls (object);

          if (cls.get_name () == cname)
            retval = true;
        }
    }

  return retval;
}

octave_value_list
octave_classdef_superclass_ref::call (octave::tree_evaluator& tw,
                                      int nargout, const octave_value_list& idx)
{
  octave_value_list retval;

  std::string meth_name;
  bool in_constructor;
  cdef_class ctx;

  ctx = get_class_context (meth_name, in_constructor);

  if (! ctx.ok ())
    error ("superclass calls can only occur in methods or constructors");

  std::string mname = m_method_name;
  std::string cname = m_class_name;

  // CLS is the superclass.  The lookup_class function handles
  // pkg.class names.

  cdef_class cls = lookup_class (cname);

  if (in_constructor)
    {
      if (! is_direct_superclass (cls, ctx))
        error ("`%s' is not a direct superclass of `%s'",
               cname.c_str (), ctx.get_name ().c_str ());

      if (! is_constructed_object (tw, mname))
        error ("cannot call superclass constructor with variable `%s'",
               mname.c_str ());

      octave_value sym = tw.varval (mname);

      cls.run_constructor (to_cdef_ref (sym), idx);

      retval(0) = sym;
    }
  else
    {
      size_t pos = mname.find ('.');

      cdef_object obj;

      if (pos != std::string::npos)
        {
          // We are looking at obj.meth.

          std::string oname = m_method_name.substr (0, pos);
          mname = mname.substr (pos + 1);

          octave_value tval = tw.varval (oname);

          // FIXME: Can we only call superclass methods on the current
          // object?  If so, and we are looking at something like
          //
          //   function meth (obj, ...)
          //     obj.meth@superclass (...)
          //
          // Do we need to verify that the object that was passed to
          // meth is the same as the object we find when looking up
          // obj in the expression obj.meth?  If so, what is the right
          // way to perform that check?

          if (tval.is_classdef_object ())
            {
              octave_classdef *cdobj = tval.classdef_object_value ();

              obj = cdobj->get_object ();
            }
        }

      if (mname != meth_name)
        error ("method name mismatch (`%s' != `%s')",
               mname.c_str (), meth_name.c_str ());

      if (! is_strict_superclass (cls, ctx))
        error ("`%s' is not a superclass of `%s'",
               cname.c_str (), ctx.get_name ().c_str ());

      // I see 2 possible implementations here:
      // 1) use cdef_object::subsref with a different class
      //    context; this avoids duplicating code, but
      //    assumes the object is always the first argument
      // 2) lookup the method manually and call
      //    cdef_method::execute; this duplicates part of
      //    logic in cdef_object::subsref, but avoid the
      //    assumption of 1)
      // Not being sure about the assumption of 1), I
      // go with option 2) for the time being.

      cdef_method meth = cls.find_method (meth_name, false);

      if (! meth.ok ())
        error ("no method `%s' found in superclass `%s'",
               meth_name.c_str (), cname.c_str ());

      retval = (obj.ok ()
                ? meth.execute (obj, idx, nargout, true, meth_name)
                : meth.execute (idx, nargout, true, meth_name));
    }

  return retval;
}

bool octave_classdef_superclass_ref::is_constructed_object (octave::tree_evaluator& tw,
                                                            const std::string& nm)
{
  octave::call_stack& cs = tw.get_call_stack ();

  octave_function *of = cs.current ();

  if (of->is_classdef_constructor ())
    {
      octave_user_function *uf = of->user_function_value (true);

      if (uf)
        {
          octave::tree_parameter_list *ret_list = uf->return_list ();

          if (ret_list && ret_list->length () == 1)
            return (ret_list->front ()->name () == nm);
        }
    }

  return false;
}

//----------------------------------------------------------------------------

DEFUN (__meta_get_package__, args, ,
       doc: /* -*- texinfo -*-
@deftypefn {} {} __meta_get_package__ ()
Undocumented internal function.
@end deftypefn */)
{
  if (args.length () != 1)
    print_usage ();

  std::string cname = args(0).xstring_value ("PACKAGE_NAME must be a string");

  return to_ov (lookup_package (cname));
}

DEFUN (metaclass, args, ,
       doc: /* -*- texinfo -*-
@deftypefn {} {} metaclass (obj)
Returns the meta.class object corresponding to the class of @var{obj}.
@end deftypefn */)
{
  if (args.length () != 1)
    print_usage ();

  cdef_object obj = to_cdef (args(0));

  return to_ov (obj.get_class ());
}

// FIXME: What about dynamic properties if obj is a scalar, or the
// properties of the class of obj if obj is an array?  Probably there
// should be a function to do this job so that the DEFUN is just a
// simple wrapper.

DEFUN (properties, args, nargout,
       doc: /* -*- texinfo -*-
@deftypefn {} {} properties (@var{class_name})
@deftypefnx {} {} properties (@var{obj})
@deftypefnx {} {@var{plist} =} properties (@dots{})
Return or display the properties for the named class @var{class_name}
or classdef object @var{obj}.  If an output value is requested, return
list of property names in a cell array.
@end deftypefn */)
{
  if (args.length () != 1)
    print_usage ();

  octave_value arg = args(0);

  std::string class_name;

  if (arg.isobject ())
    class_name = arg.class_name ();
  else if (arg.is_string ())
    class_name = arg.string_value ();
  else
    err_wrong_type_arg ("properties", arg);

  cdef_class cls;

  cls = lookup_class (class_name, false, true);

  if (! cls.ok ())
    error ("invalid class: %s", class_name.c_str ());

  std::map<std::string, cdef_property> property_map = cls.get_property_map ();

  std::list<std::string> property_names;

  for (const auto& pname_prop : property_map)
    {
      std::string nm = pname_prop.second.get_name ();

      octave_value acc = pname_prop.second.get ("GetAccess");

      if (! acc.is_string() || acc.string_value () != "public")
        continue;

      property_names.push_back (nm);
    }

  if (nargout > 0)
    return octave_value (Cell (string_vector (property_names)));

  octave_stdout << "properties for class " << class_name << ":\n\n";

  for (const auto& nm : property_names)
    octave_stdout << "  " << nm << "\n";

  octave_stdout << std::endl;

  return octave_value ();
}

/*
%!assert (properties ("inputParser"),
%!        {"CaseSensitive"; "FunctionName"; "KeepUnmatched";
%!         "Parameters"; "PartialMatching"; "Results";
%!         "StructExpand"; "Unmatched"; "UsingDefaults"});
*/

// FIXME: Need to implement the -full option.

DEFMETHOD (__methods__, interp, args, ,
           doc: /* -*- texinfo -*-
@deftypefn  {} {} __methods__ (@var{x})
@deftypefnx {} {} __methods__ ("classname")
Internal function.

Implements @code{methods} for Octave class objects and classnames.
@seealso{methods}
@end deftypefn */)
{
  // Input validation has already been done in methods.m.
  octave_value arg = args(0);

  std::string class_name;

  if (arg.isobject ())
    class_name = arg.class_name ();
  else if (arg.is_string ())
    class_name = arg.string_value ();
  else
    err_wrong_type_arg ("__methods__", arg);

  string_vector sv;

  cdef_class cls = lookup_class (class_name, false, true);

  if (cls.ok ())
    {
      std::map<std::string, cdef_method> method_map = cls.get_method_map ();

      std::list<std::string> method_names;

      for (const auto& nm_mthd : method_map)
        {
          std::string nm = nm_mthd.first;

          method_names.push_back (nm);
        }

      sv = string_vector (method_names);
    }

  // The following will also find methods for legacy @CLASS objects.

  octave::load_path& lp = interp.get_load_path ();

  sv.append (lp.methods (class_name));

  return ovl (Cell (sv));
}

/*
;;; Local Variables: ***
;;; mode: C++ ***
;;; End: ***
*/
