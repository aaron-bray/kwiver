/*ckwg +29
 * Copyright 2011-2013 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file process_factory.cxx
 *
 * \brief Python bindings for \link sprokit::process_factory\endlink.
 */

#include <sprokit/pipeline/process.h>
#include <sprokit/pipeline/process_cluster.h>
#include <sprokit/pipeline/process_factory.h>
#include <sprokit/pipeline/process_registry_exception.h>

#include <sprokit/python/util/python_exceptions.h>
#include <sprokit/python/util/python_gil.h>
#include <sprokit/python/util/python_threading.h>

#include <vital/plugin_loader/plugin_manager.h>

#include <pybind11/stl_bind.h>
#include "python_wrappers.cxx"

#ifdef WIN32
 // Windows get_pointer const volatile workaround
namespace boost
{
  template <> inline sprokit::process const volatile*
  get_pointer(class sprokit::process const volatile* p)
  {
    return p;
  }

  template <> inline sprokit::process_cluster const volatile*
  get_pointer(class sprokit::process_cluster const volatile* p)
  {
    return p;
  }
}
#endif

using namespace pybind11;

// We need our own factory for inheritance to work
// This is hopefully something pybind11 will deal with soon, and we can eliminate this class
// Otherwise, we can rewrite process_factory to have multiple entrypoints

typedef std::function< object( kwiver::vital::config_block_sptr const& config ) > py_process_factory_func_t;

class python_process_factory
: public kwiver::vital::plugin_factory
{

  public:

  python_process_factory( const std::string& type,
                          const std::string& itype,
                          py_process_factory_func_t factory );

  virtual ~python_process_factory();

  virtual object create_object(kwiver::vital::config_block_sptr const& config);

private:
  py_process_factory_func_t m_factory;
};

static void register_process( sprokit::process::type_t const& type,
                              sprokit::process::description_t const& desc,
                              object obj );

static bool is_process_loaded( const std::string& name );
static void mark_process_loaded( const std::string& name );
static std::string get_description( const std::string& name );
static std::vector< std::string > process_names();
static object create_py_process( const sprokit::process::type_t&,
                                 const sprokit::process::name_t&,
                                 const kwiver::vital::config_block_sptr);

// ==================================================================
PYBIND11_MODULE(process_factory, m)
{
  class_<sprokit::processes_t>(m, "Processes"
    , "A collection of processes.");

  bind_vector<std::vector<std::string> >(m, "StringVector");

  m.def("is_process_module_loaded", &is_process_loaded
       , (arg("module"))
       , "Returns True if the module has already been loaded, False otherwise.");

  m.def("mark_process_module_as_loaded", &mark_process_loaded
       , (arg("module"))
       , "Marks a module as loaded.");

  m.def("add_process", &register_process
      , arg("type"), arg("description"), arg("ctor")
       , "Registers a function which creates a process of the given type.");

  m.def("create_process", &create_py_process
      , arg("type"), arg("name"), arg("config") = kwiver::vital::config_block::empty_config()
      , "Creates a new process of the given type.", return_value_policy::reference_internal);

  m.def("description", &get_description
       , (arg("type"))
       , "Returns description for the process");

  m.def("types", &process_names
       , "Returns list of process names" );

  m.attr("Process") = m.import("sprokit.pipeline.process").attr("PythonProcess");
  m.attr("ProcessCluster") = m.import("sprokit.pipeline.process_cluster").attr("PythonProcessCluster");

}

python_process_factory::
python_process_factory( const std::string& type,
                        const std::string& itype,
                        py_process_factory_func_t factory )
  : plugin_factory( itype )
  , m_factory( factory )
{
  this->add_attribute( CONCRETE_TYPE, type)
    .add_attribute( PLUGIN_FACTORY_TYPE, typeid(* this ).name() )
    .add_attribute( PLUGIN_CATEGORY, "process" );
}

python_process_factory::
~python_process_factory()
{ }

object
python_process_factory::
create_object(kwiver::vital::config_block_sptr const& config)
{
  // Call sprokit factory function. Need to use this factory
  // function approach to handle clusters transparently.
  return m_factory( config );
}

object
create_py_process( const sprokit::process::type_t&         type,
                   const sprokit::process::name_t&         name,
                   const kwiver::vital::config_block_sptr  config )
{

  // First see if there's a C++ process with the name
  // If that fails, try python instead
  try
  {
    sprokit::process_t c_proc = sprokit::create_process(type, name, config);
    return cast(c_proc);
  }
  catch ( sprokit::null_process_registry_config_exception e)
  {
    throw sprokit::null_process_registry_config_exception();
  }
  catch ( const std::exception &e) // Now check python
  {
    typedef kwiver::vital::implementation_factory_by_name< object > proc_factory;
    proc_factory ifact;

    kwiver::vital::plugin_factory_handle_t a_fact;
    try
    {
      a_fact = ifact.find_factory( type );
    }
    catch ( kwiver::vital::plugin_factory_not_found& e )
    {
      auto logger = kwiver::vital::get_logger( "python_process_factory" );
      LOG_DEBUG( logger, "Plugin factory not found: " << e.what() );

      throw sprokit::no_such_process_type_exception( type );
    }

    // Add these entries to the new process config so it will know how it is instantiated.
    config->set_value( sprokit::process::config_type, kwiver::vital::config_block_value_t( type ) );
    config->set_value( sprokit::process::config_name, kwiver::vital::config_block_value_t( name ) );

    python_process_factory* pf = dynamic_cast< python_process_factory* > ( a_fact.get() );
    if (0 == pf)
    {
      // Wrong type of factory returned.
      throw sprokit::no_such_process_type_exception( type );
    }

    try
    {
      return pf->create_object( config );
    }
    catch ( const std::exception &e )
    {
      auto logger = kwiver::vital::get_logger( "python_process_factory" );
      LOG_ERROR( logger, "Exception from creating process: " << e.what() );
      throw;
    }
  }

  return none(); // we shouldn't reach this line
}

// ==================================================================
class python_process_wrapper
  : sprokit::python::python_threading
{
public:
  python_process_wrapper( object obj );
  ~python_process_wrapper();

  object operator()( kwiver::vital::config_block_sptr const& config );


private:
  object const m_obj;
};


// ------------------------------------------------------------------
void
register_process( sprokit::process::type_t const&        type,
                  sprokit::process::description_t const& desc,
                  object                                 obj )
{

  sprokit::python::python_gil const gil;

  (void)gil;

  python_process_wrapper const& wrap(obj);

  kwiver::vital::plugin_manager& vpm = kwiver::vital::plugin_manager::instance();
  auto fact = vpm.add_factory( new python_process_factory( type, // derived type name string
                                                           typeid( object ).name(),
                                                           wrap ) );

  fact->add_attribute( kwiver::vital::plugin_factory::PLUGIN_NAME, type )
    .add_attribute( kwiver::vital::plugin_factory::PLUGIN_MODULE_NAME, "python-runtime" )
    .add_attribute( kwiver::vital::plugin_factory::PLUGIN_DESCRIPTION, desc )
    ;
}


// ------------------------------------------------------------------
bool is_process_loaded( const std::string& name )
{
  kwiver::vital::plugin_manager& vpm = kwiver::vital::plugin_manager::instance();
  return vpm.is_module_loaded( name );
}


// ------------------------------------------------------------------
void mark_process_loaded( const std::string& name )
{
  kwiver::vital::plugin_manager& vpm = kwiver::vital::plugin_manager::instance();
  vpm.mark_module_as_loaded( name );
}


// ------------------------------------------------------------------
std::string get_description( const std::string& type )
{
  kwiver::vital::plugin_factory_handle_t a_fact;
  try
  {
    typedef kwiver::vital::implementation_factory_by_name< sprokit::process > proc_factory;
    proc_factory ifact;

    SPROKIT_PYTHON_TRANSLATE_EXCEPTION(
      a_fact = ifact.find_factory( type );
      )

  }
  catch ( const std::exception &e )
  {
    typedef kwiver::vital::implementation_factory_by_name< object > py_proc_factory;
    py_proc_factory ifact;

    SPROKIT_PYTHON_TRANSLATE_EXCEPTION(
      a_fact = ifact.find_factory( type );
      )
  }


  std::string buf = "-- Not Set --";
  a_fact->get_attribute( kwiver::vital::plugin_factory::PLUGIN_DESCRIPTION, buf );

  return buf;
}


// ------------------------------------------------------------------
std::vector< std::string > process_names()
{
  kwiver::vital::plugin_manager& vpm = kwiver::vital::plugin_manager::instance();
  auto py_fact_list = vpm.get_factories<object>();

  std::vector<std::string> name_list;
  for( auto fact : py_fact_list )
  {
    std::string buf;
    if (fact->get_attribute( kwiver::vital::plugin_factory::PLUGIN_NAME, buf ))
    {
      name_list.push_back( buf );
    }
  } // end foreach

  auto fact_list = vpm.get_factories<sprokit::process>();

  for( auto fact : py_fact_list )
  {
    std::string buf;
    if (fact->get_attribute( kwiver::vital::plugin_factory::PLUGIN_NAME, buf ))
    {
      name_list.push_back( buf );
    }
  } // end foreach

  return name_list;
}

// ------------------------------------------------------------------
python_process_wrapper
  ::python_process_wrapper( object obj )
  : m_obj( object(obj) )
{
}


python_process_wrapper
  ::~python_process_wrapper()
{
}


// ------------------------------------------------------------------
object
python_process_wrapper
  ::operator()( kwiver::vital::config_block_sptr const& config )
{
  sprokit::python::python_gil const gil;

  (void)gil;

  return m_obj( config );
}
