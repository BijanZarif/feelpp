/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t  -*-
 
 This file is part of the Feel++ library
 
 Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
 Date: 16 Mar 2015
 
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
#ifndef FEELPP_MODELMATERIALS_HPP
#define FEELPP_MODELMATERIALS_HPP 1


#include <vector>

#include <boost/property_tree/ptree.hpp>

namespace Feel {

namespace pt =  boost::property_tree;

struct ModelMaterial
{
    ModelMaterial() = default;
    ModelMaterial( ModelMaterial const& ) = default;
    ModelMaterial( ModelMaterial&& ) = default;
    ModelMaterial& operator=( ModelMaterial const& ) = default;
    ModelMaterial& operator=( ModelMaterial && ) = default;
    ModelMaterial( std::string name )
        :
        M_name( name ),
        M_rho( 1. ),
        M_mu(1. ),
        M_Cp( 1 ),
        M_Cv( 1 ),
        M_k11( 1 ),
        M_k12( 0 ),
        M_k13( 0 ),
        M_k22( 1 ),
        M_k23( 0 ),
        M_k33( 1 ),
        M_Tref( 0 ),
        M_beta( 0 ),
        M_C( 1 ),
        M_young_modulus( 1 ),
        M_nu( 1 ),
        M_sigma( 1 )
        {}
    std::string const& name() const { return M_name; }
    void setName( std::string const& name ) { M_name = name; }

    // material mass density
    double rho() const { return M_rho; }
    void setRho( double v ) { M_rho = v; }

    // Molecular(dynamic) viscosity
    double mu() const { return M_mu; }
    void setMu( double v ) { M_mu = v; }
    
    //Specify the constant-pressure specific heat Cp.
    double Cp() const {  return M_Cp; }
    void setCp( double t) { M_Cp = t; }
    //Specify the constant-volume specific heat Cv.
    double Cv() const {  return M_Cv; }
    void setCv( double t) { M_Cv = t; }

    // heat diffusion coefficients
    double k11() const {  return M_k11; }
    void setK11( double t) { M_k11 = t; }
    double k12() const {  return M_k12; }
    void setK12( double t) { M_k12 = t; }
    double k13() const {  return M_k13; }
    void setK13( double t) { M_k13 = t; }
    double k22() const {  return M_k22; }
    void setK22( double  t) { M_k22 = t; }
    double k23() const {  return M_k23; }
    void setK23( double t) { M_k23 = t; }
    double k33() const {  return M_k33; }
    void setK33( double t) { M_k33 = t; }
    
    // Material Reference temperature
    double Tref() const {  return M_Tref; }
    void setTref( double t) { M_Tref = t; }
    
    // Material coefficient for thermal expansion
    double beta() const {  return M_beta; }
    void setBeta( double t) { M_beta = t; }

    // heat capacity
    double C() const {  return M_C; }
    void setC( double const& t) { M_C = t; }
    
    // Mechanical properties
    // Young's Modulus
    double E() const {  return M_young_modulus; }
    void setE( double const& t) { M_young_modulus = t; }

    // Poisson's ratio
    double nu() const {  return M_nu; }
    void setNu( double const& t) { M_nu = t; }
    // electrical conductivity
    double sigma() const {  return M_sigma; }
    void setSigma( double const& t) { M_sigma = t; }
    

    void load( std::string const& );
private:
    std::string M_name;
    double M_rho;
    double M_mu;
    
    double M_Cp;
    double M_Cv;

    // Thermal properties
    double M_k11, M_k12, M_k13, M_k22, M_k23, M_k33;
    double M_Tref;
    double M_beta;
    double M_C;
    
    // Mechanical Properties
    // Young's Modulus
    double M_young_modulus;
    // Poisson's Ratio
    double M_nu;

    // Electrical conductivity
    double M_sigma;
};

std::ostream& operator<<( std::ostream& os, ModelMaterial const& m );

/**
 * @brief a set of materials
 * key: mesh marker
 * name -> name of the materials - can be different
 */
class ModelMaterials: public std::map<std::string,ModelMaterial>
{
public:
    using value_type = std::map<std::string,ModelMaterial>::value_type;
    ModelMaterials() = default;
    ModelMaterials( pt::ptree const& p );
    virtual ~ModelMaterials() = default;
    void setPTree( pt::ptree const& _p ) { M_p = _p; setup(); }
    ModelMaterial loadMaterial( std::string const& );
    ModelMaterial getMaterial( pt::ptree const& );

    ModelMaterial const&
    material( std::string const& m ) const
        {
            auto it = this->find( m );
            if ( it == this->end() )
                throw std::invalid_argument( std::string("ModelMaterial: Invalid material marker ") + m );
            return it->second;
            
        }
    void saveMD(std::ostream &os);
private:
    void setup();
private:
    pt::ptree M_p;
};

inline ModelMaterial
material( ModelMaterials::value_type const& m )
{
    return m.second;
}

inline std::string
marker( ModelMaterials::value_type const& m )
{
    return m.first;
}

}
#endif
