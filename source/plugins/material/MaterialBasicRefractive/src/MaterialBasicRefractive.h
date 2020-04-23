/***************************************************************************
Copyright (C) 2008 by the Tonatiuh Software Development Team.

This file is part of Tonatiuh.

Tonatiuh program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


Acknowledgments:

The development of Tonatiuh was started on 2004 by Dr. Manuel J. Blanco,
then Chair of the Department of Engineering of the University of Texas at
Brownsville. From May 2004 to July 2008, it was supported by the Department
of Energy (DOE) and the National Renewable Energy Laboratory (NREL) under
the Minority Research Associate (MURA) Program Subcontract ACQ-4-33623-06.
During 2007, NREL also contributed to the validation of Tonatiuh under the
framework of the Memorandum of Understanding signed with the Spanish
National Renewable Energy Centre (CENER) on February, 20, 2007 (MOU#NREL-07-117).
Since June 2006, the development of Tonatiuh is being led by the CENER, under the
direction of Dr. Blanco, now Director of CENER Solar Thermal Energy Department.

Developers: Manuel J. Blanco (mblanco@cener.com), Amaia Mutuberria, Victor Martin.

Contributors: Javier Garcia-Barberena, Inaki Perez, Inigo Pagola,  Gilda Jimenez,
Juana Amieva, Azael Mancillas, Cesar Cantu.
***************************************************************************/

#ifndef MaterialBasicRefractive_H_
#define MaterialBasicRefractive_H_

#include <Inventor/fields/SoSFDouble.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFString.h>

#include "TMaterial.h"
#include "trt.h"

class SoSensor;

class MaterialBasicRefractive : public TMaterial
{
	SO_NODE_HEADER(MaterialBasicRefractive);

public:
	enum Distribution {
		//PILLBOX = 0,
		NORMAL = 1,
	   };
	MaterialBasicRefractive( );
	static void initClass();

    QString getIcon();
	//Ray* OutputRay( const Ray& incident, DifferentialGeometry* dg, RandomDeviate& rand  ) const;
    bool OutputRay( const Ray& incident, DifferentialGeometry* dg, RandomDeviate& rand, Ray* outputRay  ) const;

	trt::TONATIUH_REAL reflectivityFront;
	trt::TONATIUH_REAL reflectivityBack;
	trt::TONATIUH_REAL transmissivityFront;
	trt::TONATIUH_REAL transmissivityBack;
	trt::TONATIUH_REAL nFront;
	trt::TONATIUH_REAL nBack;
	trt::TONATIUH_REAL sigmaSlope;
	//trt::TONATIUH_REAL m_sigmaSpecularity; ** yet to implemented
	SoSFEnum distribution;
	SoMFColor m_ambientColor;
	SoMFColor m_diffuseColor;
	SoMFColor m_specularColor;
	SoMFColor m_emissiveColor;
	SoMFFloat m_shininess;
	SoMFFloat m_transparency;


protected:
   	virtual ~MaterialBasicRefractive();

   	double m_sigmaOpt;

	static void updateReflectivityFront( void* data, SoSensor* );
	static void updateTransmissivityFront( void* data, SoSensor* );
	static void updateReflectivityBack( void* data, SoSensor* );
	static void updateTransmissivityBack( void* data, SoSensor* );
	static void updateAmbientColor( void* data, SoSensor* );
	static void updateDiffuseColor( void* data, SoSensor* );
	static void updateSpecularColor( void* data, SoSensor* );
	static void updateEmissiveColor( void* data, SoSensor* );
	static void updateShininess( void* data, SoSensor* );
	static void updateTransparency( void* data, SoSensor* );

	Ray ReflectedRay( const Ray& incident, DifferentialGeometry* dg, RandomDeviate& rand  ) const;
	Ray RefractedtRay( const Ray& incident, DifferentialGeometry* dg, RandomDeviate& rand  ) const;

};

#endif /*MaterialBasicRefractive_H_*/
