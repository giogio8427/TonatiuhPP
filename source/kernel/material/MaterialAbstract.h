#pragma once

#include "kernel/TonatiuhKernel.h"
#include <Inventor/nodes/SoMaterial.h>

struct DifferentialGeometry;
class RandomDeviate;
class Ray;
class QString;

class TONATIUH_KERNEL MaterialAbstract: public SoMaterial
{
    SO_NODE_ABSTRACT_HEADER(MaterialAbstract);

public:
    static void initClass();

    virtual QString getIcon() = 0;
    virtual bool OutputRay(const Ray& incident, DifferentialGeometry* dg, RandomDeviate& rand, Ray* outputRay  ) const = 0;

    static const char* getClassName() {return "Material";}

protected:
    MaterialAbstract();
    virtual ~MaterialAbstract();
};