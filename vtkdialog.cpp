#include "vtkdialog.h"
#include "ui_vtkdialog.h"
#include "QFileDialog"
#include "utilities.h"
#include "QDebug"
#ifdef use_VTK
#include "VTK.h"
#endif


VTKDialog::VTKDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VTKDialog)
{
    ui->setupUi(this);
    connect(ui->ChooseOutputFile,SIGNAL(clicked()),this,SLOT(onReadResults()));
    connect(ui->BtmModelScript,SIGNAL(clicked()),this,SLOT(onReadScript()));
    connect(this,SIGNAL(accepted()),this,SLOT(onGenerateVTK()));
}

VTKDialog::~VTKDialog()
{
    delete ui;
}

void VTKDialog::onReadResults()
{
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open"), "",
            tr("text files (*.txt)"));

    ui->TxtOutPutFileName->setText(fileName);
    AllResults.getfromfile(fileName.toStdString(),true);
    for (unsigned int i=0; i<AllResults.nvars; i++)
    {
        if (!variables.contains(QString::fromStdString(AllResults.names[i]).split("_").last()))
            variables.append(QString::fromStdString(AllResults.names[i]).split("_").last());
    }

    ui->VariablecomboBox->addItems(variables);

}
void VTKDialog::onReadScript()
{
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open"), "",
            tr("OpenHydroQual Script (*.ohq)"));
    ui->txtModelScriptFile->setText(fileName);

    QFile inputfile(fileName);
    if (!inputfile.open(QIODevice::ReadOnly|QIODevice::Text))
        qDebug()<< "file "<<fileName<< " cannot be opened";
    else
        qDebug()<<fileName<< " successfully opened!";

    QTextStream in(&inputfile);

    while (!in.atEnd())
       {
            QString line = in.readLine();
            QString command = line.split(";").first();
            if (command=="create block")
            {
                QStringList attributes = line.split(";").last().split(",");
                blockinfo blkinf;
                blkinf.location.setX(-999);
                blkinf.location.setY(-999);
                for (unsigned int j = 0; j<attributes.count(); j++)
                {

                    qDebug()<<attributes[j].split("=").first();
                    if (attributes[j].split("=").first()=="name")
                        blkinf.name = attributes[j].split("=").last();
                    if (attributes[j].split("=").first()=="actual_x")
                        blkinf.location.setX(attributes[j].split("=").last().toDouble());
                    if (attributes[j].split("=").first()=="actual_y")
                        blkinf.location.setY(attributes[j].split("=").last().toDouble());
                }
                qDebug()<<blkinf.name<<":"<<blkinf.location;
                BlockInfo[blkinf.name] = blkinf;
            }

       }
    inputfile.close();
}
void VTKDialog::onGenerateVTK()
{
    QString fileName = QFileDialog::getSaveFileName(this,
               tr("vtp"), "",
               tr("Paraview Grid (*.vtp)"));

    qDebug()<<fileName<< " successfully opened!";
    QVector<cellplotinfo> PlotInfo;
    CTimeSeriesSet<double> toplottimeseries;
    for (int i=0; i<AllResults.nvars; i++)
    {

        if (QString::fromStdString(AllResults.names[i]).split("_").last()==ui->VariablecomboBox->currentText())
        {
            cellplotinfo pltinf;
            QString blkname = QString::fromStdString(AllResults.names[i]).left(QString::fromStdString(AllResults.names[i]).size()- QString::fromStdString(AllResults.names[i]).split("_").last().size()-1);
            qDebug()<<blkname;
            if (BlockInfo.contains(blkname))
            {   pltinf.name_location = BlockInfo[blkname];
                pltinf.column_no = i;

               if (BlockInfo[blkname].location.x()!=-999)
                {   PlotInfo.append(pltinf);
                    toplottimeseries.append(AllResults.BTC[i],AllResults.names[i]);
                    qDebug()<<pltinf.name_location.name<<":"<<pltinf.name_location.location;
                }
            }
        }
    }

    if (toplottimeseries.nvars == 0) return;
    int i=0;
    for (double t = toplottimeseries.BTC[0].GetT(0); t<toplottimeseries.BTC[0].GetT(toplottimeseries.BTC[0].n-1); t+=ui->lineEditInterval->text().toDouble())
    {
        vector<double> vals = toplottimeseries.interpolate(t);
#ifdef use_VTK
             QVector<double> values = ConvertToQVector(vals);
        #endif
             qDebug()<<fileName;
             QString filename = fileName.split(".")[0] + "_" + QString::number(i) + "." + fileName.split(".")[1];
             qDebug()<<filename;
    qDebug()<<i<<":"<<t;
#ifdef use_VTK
    qDebug()<<PlotInfo.count()<<","<<values.count();
        write_to_vtp(PlotInfo,values,filename,ui->VariablecomboBox->currentText());

#endif
        i++;
    }
}

#ifdef use_VTK
void VTKDialog::write_to_vtp(const QVector<cellplotinfo> &cellinfo, QVector<double> &_values, QString &filename, const QString &name)
{
    if (cellinfo.count()==0) return;
    vtkSmartPointer<vtkPoints> points_3 =
            vtkSmartPointer<vtkPoints>::New();

        double xx, yy, zz;
        vtkSmartPointer<vtkFloatArray> values =
            vtkSmartPointer<vtkFloatArray>::New();

        values->SetNumberOfComponents(1);

        values->SetName(name.toStdString().c_str());



        for (unsigned int i = 0; i < _values.size(); i++)
        {
            xx = cellinfo[i].name_location.location.x();
            yy = cellinfo[i].name_location.location.y();
            zz = _values[i];


            float t[1] = { float(_values[i]) };
            points_3->InsertNextPoint(xx, yy, zz);
            values->InsertNextTupleValue(t);

        }


        // Add the grid points to a polydata object
        vtkSmartPointer<vtkPolyData> inputPolyData =
            vtkSmartPointer<vtkPolyData>::New();
        inputPolyData->SetPoints(points_3);

        // Triangulate the grid points
        vtkSmartPointer<vtkDelaunay2D> delaunay =
            vtkSmartPointer<vtkDelaunay2D>::New();
    #if VTK_MAJOR_VERSION <= 5
        delaunay->SetInput(inputPolyData);
    #else
        delaunay->SetInputData(inputPolyData);
    #endif
        delaunay->Update();
        vtkPolyData* outputPolyData = delaunay->GetOutput();

        double bounds[6];
        outputPolyData->GetBounds(bounds);

        // Find min and max z
        double minz = bounds[4];
        double maxz = bounds[5];

        // Create the color map
        vtkSmartPointer<vtkLookupTable> colorLookupTable =
            vtkSmartPointer<vtkLookupTable>::New();
        colorLookupTable->SetTableRange(minz, maxz);
        colorLookupTable->Build();

        // Generate the colors for each point based on the color map
        vtkSmartPointer<vtkUnsignedCharArray> colors_2 =
            vtkSmartPointer<vtkUnsignedCharArray>::New();
        colors_2->SetNumberOfComponents(3);
        colors_2->SetName("Colors");


        for (int i = 0; i < outputPolyData->GetNumberOfPoints(); i++)
        {
            double p[3];
            outputPolyData->GetPoint(i, p);

            double dcolor[3];
            colorLookupTable->GetColor(p[2], dcolor);

            unsigned char color[3];
            for (unsigned int j = 0; j < 3; j++)
            {
                color[j] = static_cast<unsigned char>(255.0 * dcolor[j]);
            }
            //std::cout << "color: "
            //	<< (int)color[0] << " "
            //	<< (int)color[1] << " "
            //	<< (int)color[2] << std::endl;

            colors_2->InsertNextTupleValue(color);
        }

        outputPolyData->GetPointData()->SetScalars(values);


        //Append the two meshes
        vtkSmartPointer<vtkAppendPolyData> appendFilter =
            vtkSmartPointer<vtkAppendPolyData>::New();
    #if VTK_MAJOR_VERSION <= 5
        appendFilter->AddInputConnection(input1->GetProducerPort());
        appendFilter->AddInputConnection(input2->GetProducerPort());
    #else
        //appendFilter->AddInputData(polydata);
        //appendFilter->AddInputData(polydata_1);
        appendFilter->AddInputData(outputPolyData);
    #endif
        appendFilter->Update();


        // Visualization
        vtkSmartPointer<vtkPolyDataMapper> mapper =
            vtkSmartPointer<vtkPolyDataMapper>::New();
    #if VTK_MAJOR_VERSION <= 5
        mapper->SetInputConnection(polydata->GetProducerPort());
    #else
        mapper->SetInputConnection(appendFilter->GetOutputPort());
        //mapper->SetInputData(polydata_1);
    #endif

        vtkSmartPointer<vtkActor> actor =
            vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetPointSize(5);

        vtkSmartPointer<vtkXMLPolyDataWriter> writer =
            vtkSmartPointer<vtkXMLPolyDataWriter>::New();
        writer->SetFileName(filename.toStdString().c_str());
        writer->SetInputData(mapper->GetInput());
        // This is set so we can see the data in a text editor.
        writer->SetDataModeToAscii();
        writer->Write();

}

#endif

QVector<double> ConvertToQVector(const vector<double> &x)
{
    QVector<double> out;
    for (unsigned int i=0; i<x.size(); i++)
        out.push_back(x[i]);

    return out;
}

