/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t  -*-
 
 This file is part of the Feel++ library
 
 Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
 Date: 15 Mar 2015
 
 Copyright (C) 2015 Feel++ Consortium
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef FEELPP_MODELPROPERTIES_HPP
#define FEELPP_MODELPROPERTIES_HPP 1

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <feel/feelmodels/modelparameters.hpp>
#include <feel/feelmodels/modelmaterials.hpp>
#include <feel/feelmodels/modelpostprocess.hpp>
#include <feel/feelpde/boundaryconditions.hpp>


namespace Feel {

namespace pt =  boost::property_tree;

class ModelProperties
{
public:
    ModelProperties( std::string const& filename );
    virtual ~ModelProperties();

    std::string const& name() const {  return M_name; }
    void setName( std::string const& t) { M_name = t; }
    std::string shortName() const {  return M_shortname; }
    void setShortName( std::string const& t) { M_shortname = t; }
    

    std::string const& description() const {  return M_description; }
    void setDescription( std::string const& t) { M_description = t; }

    std::string const& model() const {  return M_model; }
    void setModel( std::string const& t) { M_model = t; }
    

    ModelParameters const& parameters() const {  return M_params; }
    ModelMaterials const& materials() const {  return M_mat; }
    BoundaryConditions const& boundaryConditions() const { return M_bc; }
    
    ModelParameters & parameters()  {  return M_params; }
    ModelMaterials & materials() {  return M_mat; }
    BoundaryConditions & boundaryConditions()  { return M_bc; }

    ModelPostprocess& postProcess() { return M_postproc; }
    ModelPostprocess const& postProcess() const { return M_postproc; }

    std::string getEntry(std::string &s);

    void saveMD(std::ostream &os);

    /**
     * Add an entry to the tree
     * @param[in] key Where is stored the value in the tree
     * @param[in] entry The value of the key
     **/
    void put( std::string const &key, std::string const &entry);

    /**
     * Save the stree
     * @param[in] filename The file to save the current tree
     **/
    void write(std::string const &filename);
    
private:
    pt::ptree M_p;
    std::string M_name, M_shortname, M_description, M_model;
    ModelParameters M_params;
    ModelMaterials M_mat;
    BoundaryConditions M_bc;
    ModelPostprocess M_postproc;
};


}
#endif
