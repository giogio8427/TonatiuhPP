#pragma once

#include "kernel/scene/TAbstract.h"
#include <Inventor/nodes/SoSubNode.h>


struct DifferentialGeometry;
class Random;
class Ray;


class TONATIUH_KERNEL Aperture: public SoNode
{
    SO_NODE_ABSTRACT_HEADER(Aperture);

public:
    static void initClass();

    virtual bool isInside(double /*x*/, double /*y*/) const {return true;}

    NAME_ICON_FUNCTIONS("X", ":/MaterialX.png")

protected:
    Aperture() {}
    ~Aperture() {}
};



#include "kernel/scene/TFactory.h"

class ApertureFactory: public TFactory
{
public:
    virtual Aperture* create() const = 0;
};

Q_DECLARE_INTERFACE(ApertureFactory, "tonatiuh.ApertureFactory")


template<class T>
class ApertureFactoryT: public ApertureFactory
{
public:
    QString name() const {return T::getClassName();}
    QIcon icon() const {return QIcon(T::getClassIcon());}
    void init() const {T::initClass();}
    T* create() const {return new T;}
};