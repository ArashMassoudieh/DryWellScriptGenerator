#ifndef SCAD_GENERATOR_H
#define SCAD_GENERATOR_H

#include <QString>
#include <QVector3D>
#include <QVector>

class SCAD_Generator
{
public:
    SCAD_Generator();
    QString GenerateTube(QVector3D basecenter, QVector<double> color, double outerradius, double innerradius, double height);
    QString GenerateCylinder(QVector3D basecenter, QVector<double> color, double outerradius, double height);
    QString GenerateBox(QVector3D basecenter, QVector3D size, QVector<double> color);
    QString GenerateBoxFC(QString name, QVector3D basecenter, QVector3D size, QVector<double> color);
    QString GenerateCylinderFC(QString name, QVector3D basecenter, QVector<double> color, double outerradius, double height);
    QString GenerateTubeFC(QString name, QVector3D basecenter, QVector<double> color, double outerradius, double innerradius, double height);
    QString CreateWireFC(QString name, QVector<QVector3D> points, bool closed = true);
    QString CreatePondFC(QString name, double coefficient, double exponent, double z_base, double radius, bool closed, QVector<double> color, unsigned int num_segments=10);
    QString BooleanCut(QString name, QString from, QString tool, QVector<double> color);
    bool colorit = false;
};

#endif // SCAD_GENERATOR_H
