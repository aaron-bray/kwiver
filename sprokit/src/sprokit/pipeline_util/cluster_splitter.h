/*ckwg +29
 * Copyright 2016 by Kitware, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
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
 * @file   cluster_splitter.h
 * @brief  Interface for cluster_splitter class
 */

#ifndef SPROKIT_PIPELINE_UTIL_CLUSTER_SPLITTER_H
#define SPROKIT_PIPELINE_UTIL_CLUSTER_SPLITTER_H

#include "cluster_bakery.h"


namespace sprokit {

// ----------------------------------------------------------------
/**
 * @brief Separate cluster blocks by type.
 *
 * This class/visitor separates each cluster block type into its own
 * list.
 */
  class cluster_splitter
    : public boost::static_visitor<>
  {
  public:
    cluster_splitter(cluster_bakery::cluster_component_info_t& info);
    ~cluster_splitter();

    void operator () (cluster_config_t const& config_block);
    void operator () (cluster_input_t const& input_block);
    void operator () (cluster_output_t const& output_block);

    // is filled in by visitor.
    cluster_bakery::cluster_component_info_t& m_info;

  private:
    typedef std::set<process::port_t> unique_ports_t;

    unique_ports_t m_input_ports;
    unique_ports_t m_output_ports;
  };

} // end namespace sprokit


#endif /* SPROKIT_PIPELINE_UTIL_CLUSTER_SPLITTER_H */
