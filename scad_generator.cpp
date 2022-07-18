#include "scad_generator.h"
#include <QDebug>
#include <math.h>
SCAD_Generator::SCAD_Generator()
{

}

QString SCAD_Generator::GenerateTube(QVector3D basecenter, QVector<double> color, double outerradius, double innerradius, double height)
{
    if (color.count()!=4)
    {
        qDebug() << "color must have four elements";
        return QString();
    }
    QString out;
    out += QString("color(["+QString::number(color[0])+", "+QString::number(color[1])+", "+QString::number(color[2])+", "+QString::number(color[3])+"]) {\n");
    out += QString("\tdifference() {\n");
    out += QString("\t\tmultmatrix([[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, "+QString::number(basecenter[2])+"], [0, 0, 0, 1]]) {\n");
    out += QString("\t\t\tcylinder($fn = 0, $fa = 12, $fs = 2, h = "+QString::number(height)+", r1 = "+QString::number(outerradius)+", r2 = "+QString::number(outerradius)+", center = false);}\n");
    out += QString("\t\tmultmatrix([[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, "+QString::number(basecenter[2])+"], [0, 0, 0, 1]]) {\n");
    out += QString("\t\t\tcylinder($fn = 0, $fa = 12, $fs = 2, h = "+QString::number(height)+", r1 = "+QString::number(innerradius)+", r2 = "+QString::number(innerradius)+", center = false);}\n");
    out += QString("\t}\n");
    out += QString("}\n");
    return out;
}

QString SCAD_Generator::GenerateCylinder(QVector3D basecenter, QVector<double> color, double outerradius, double height)
{
    if (color.count()!=4)
    {
        qDebug() << "color must have four elements";
        return QString();
    }
    QString out;
    out += QString("\t\tmultmatrix([[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, "+QString::number(basecenter[2])+"], [0, 0, 0, 1]]) {\n");
    out += QString("\t\t\tcylinder($fn = 0, $fa = 12, $fs = 2, h = "+QString::number(height)+", r1 = "+QString::number(outerradius)+", r2 = "+QString::number(outerradius)+", center = false);}\n");
    return out;
}

QString SCAD_Generator::GenerateBox(QVector3D basecenter, QVector3D size, QVector<double> color)
{
    if (color.count()!=4)
    {
        qDebug() << "color must have four elements";
        return QString();
    }
    QString out;
    out += QString("color(["+QString::number(color[0])+", "+QString::number(color[1])+", "+QString::number(color[2])+", "+QString::number(color[3])+"]) {\n");
    out += QString("\tmultmatrix([[1, 0, 0, "+QString::number(basecenter.x()-size.x()/2)+"], [0, 1, 0, "+QString::number(basecenter.y()-size.y()/2)+"], [0, 0, 1, "+QString::number(basecenter.z())+"], [0, 0, 0, 1]]) {\n");
    out += QString("\t\tcube(size = ["+QString::number(size.x())+", "+QString::number(size.y())+", "+QString::number(size.z())+"], center = false);\n");
    out += QString("\t}\n");
    out += QString("}\n");
    return out;

}

QString SCAD_Generator::GenerateBoxFC(QString name, QVector3D basecenter, QVector3D size, QVector<double> color)
{
    if (color.count()!=4)
    {
        qDebug() << "color must have four elements";
        return QString();
    }
    QString out;
    out += QString("App.ActiveDocument.addObject(\"Part::Box\",\""+name+"\")\n");
    out += QString("App.ActiveDocument.getObject(\""+name+"\").Label = \""+name+"\"\n");
    out += QString("App.ActiveDocument."+name+".Placement=App.Placement(App.Vector("+QString::number(basecenter[0]-size.x()/2)+","+QString::number(basecenter[1]-size.y()/2)+","+QString::number(basecenter[2])+"), App.Rotation(App.Vector(0,0,1),0), App.Vector(0,0,0))\n");
    out += QString("App.ActiveDocument."+name+".Length="+QString::number(size[0])+"\n");
    out += QString("App.ActiveDocument."+name+".Width="+QString::number(size[1])+"\n");
    out += QString("App.ActiveDocument."+name+".Height="+QString::number(size[2])+"\n");
    if (colorit)
        out += QString("App.ActiveDocument."+name+".ViewObject.ShapeColor = ("+QString::number(color[0],'f',2)+", "+QString::number(color[1],'f',2)+", "+QString::number(color[2],'f',2)+", "+QString::number(color[3],'f',2)+")\n");
    return out;

}

QString SCAD_Generator::GenerateCylinderFC(QString name, QVector3D basecenter, QVector<double> color, double outerradius, double height)
{
    QString out;
    out += QString("App.ActiveDocument.addObject(\"Part::Cylinder\",\""+name+"\")\n");
    out += QString("App.ActiveDocument.getObject(\""+name+"\").Label = \""+name+"\"\n");
    out += QString("App.ActiveDocument."+name+".Placement=App.Placement(App.Vector("+QString::number(basecenter[0])+","+QString::number(basecenter[1])+","+QString::number(basecenter[2])+"), App.Rotation(App.Vector(0,0,1),0), App.Vector(0,0,0))\n");
    out += QString("App.ActiveDocument."+name+".Radius="+QString::number(outerradius)+"\n");
    out += QString("App.ActiveDocument."+name+".Height="+QString::number(height)+"\n");
    out += QString("App.ActiveDocument."+name+".Angle=360.00\n");
    if (colorit)
        out += QString("App.ActiveDocument."+name+".ViewObject.ShapeColor = ("+QString::number(color[0],'f',2)+", "+QString::number(color[1],'f',2)+", "+QString::number(color[2],'f',2)+", "+QString::number(color[3],'f',2)+")\n");
    return out;
}

QString SCAD_Generator::GenerateTubeFC(QString name, QVector3D basecenter, QVector<double> color, double outerradius, double innerradius, double height)
{
    QString out;
    out += GenerateCylinderFC(name+"_inner",basecenter,color,innerradius,height);
    out += GenerateCylinderFC(name+"_outer",basecenter,color,outerradius,height);
    out += QString("App.ActiveDocument.addObject(\"Part::Cut\",\""+name+"\")\n");
    out += QString("App.ActiveDocument."+name+".Base = App.ActiveDocument."+name+"_outer"+"\n");
    out += QString("App.ActiveDocument."+name+".Tool = App.ActiveDocument."+name+"_inner"+"\n");
    out += QString("Gui.ActiveDocument."+name+"_outer"+".Visibility=False\n");
    out += QString("Gui.ActiveDocument."+name+"_inner"+".Visibility=False\n");
    if (colorit)
        out += QString("App.ActiveDocument.getObject('"+name+"').ViewObject.ShapeColor=getattr(App.ActiveDocument.getObject('"+name+"_outer"+"').getLinkedObject(True).ViewObject,'ShapeColor',App.ActiveDocument.getObject('"+name+"').ViewObject.ShapeColor)\n");
    out += QString("App.ActiveDocument.getObject('"+name+"').ViewObject.DisplayMode=getattr(App.ActiveDocument.getObject('"+name+"_outer"+"').getLinkedObject(True).ViewObject,'DisplayMode',App.ActiveDocument.getObject('"+name+"').ViewObject.DisplayMode)\n");
    return  out;
}

QString SCAD_Generator::CreateWireFC(QString name, QVector<QVector3D> points, bool closed)
{
    QString out;
    out += "pts = []\n";
    for (unsigned int i=0; i<points.size(); i++)
        out += "pts.append(App.Vector("+QString::number(points[i].x())+","+QString::number(points[i].y())+","+QString::number(points[i].z())+"))\n";

    if (closed)
        out += name + " = Draft.make_wire(pts, closed=True)\n";
    else
        out += name + " = Draft.make_wire(pts, closed=True)\n";
    out += name + ".Label = \""+name+"\"\n";
    return out;
}

QString SCAD_Generator::CreatePondFC(QString name, double coefficient, double exponent, double z_base, double radius, bool closed, QVector<double> color, unsigned int num_segments)
{
    QString out;
    QVector<QVector3D> points;
    double dr = radius/double(num_segments);
    for (unsigned int i=0; i<num_segments+1; i++)
    {
        QVector3D pt(dr*i,0,z_base+coefficient*pow(dr*i,exponent));
        points.append(pt);
    }
    if (closed)
        points.append(QVector3D(0,0,z_base+coefficient*pow(radius,exponent)));
    else
    {   points.append(QVector3D(radius,0,z_base-0.01));
        points.append(QVector3D(0,0,z_base-0.01));
    }
    out += CreateWireFC(name+"_wire",points,closed);
    out += QString("App.ActiveDocument.addObject(\"Part::Revolution\",\""+name+"\")\n");
    out += QString("App.ActiveDocument."+name+".Source = "+name+"_wire"+"\n");
    out += QString("App.ActiveDocument."+name+".Axis = (0.000000000000000,0.000000000000000,1.000000000000000)\n");
    out += QString("App.ActiveDocument."+name+".Base = (0.000000000000000,0.000000000000000,0.000000000000000)\n");
    out += QString("App.ActiveDocument."+name+".Angle = 360.000000000000000\n");
    if (closed)
        out += QString("App.ActiveDocument."+name+".Solid = True\n");
    else
        out += QString("App.ActiveDocument."+name+".Solid = True\n");
    out += QString("App.ActiveDocument."+name+".AxisLink = None\n");
    out += QString("App.ActiveDocument."+name+".Symmetric = False\n");
    if (colorit)
        out += QString("App.ActiveDocument."+name+".ViewObject.ShapeColor = ("+QString::number(color[0],'f',2)+", "+QString::number(color[1],'f',2)+", "+QString::number(color[2],'f',2)+", "+QString::number(color[3],'f',2)+")\n");
    out += QString(name+"_wire"+".Visibility = False\n");
    return out;

}

QString SCAD_Generator::BooleanCut(QString name, QString from, QString tool, QVector<double> color)
{
    QString out;
    out += QString("App.ActiveDocument.addObject(\"Part::Cut\",\""+name+"\")\n");
    out += QString("App.ActiveDocument."+name+".Base = App.ActiveDocument."+from+"\n");
    out += QString("App.ActiveDocument."+name+".Tool = App.ActiveDocument."+tool+"\n");
    out += QString("Gui.ActiveDocument."+from+".Visibility=False\n");
    out += QString("Gui.ActiveDocument."+tool+".Visibility=False\n");
    if (colorit)
        out += QString("App.ActiveDocument.getObject('"+name+"').ViewObject.ShapeColor=("+QString::number(color[0],'f',2)+", "+QString::number(color[1],'f',2)+", "+QString::number(color[2],'f',2)+", "+QString::number(color[3],'f',2)+")\n");;
    out += QString("App.ActiveDocument.getObject('"+name+"').ViewObject.DisplayMode=getattr(App.ActiveDocument.getObject('"+from+"').getLinkedObject(True).ViewObject,'DisplayMode',App.ActiveDocument.getObject('"+name+"').ViewObject.DisplayMode)\n");
    return out;

}
