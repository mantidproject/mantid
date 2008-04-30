#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include "settingseditor.h"

SettingsEditor::SettingsEditor(QWidget *parent):
    QFrame(parent)
{
    QGroupBox *axesBox = new QGroupBox("Axes", this);
    QVBoxLayout* axesBoxLayout = new QVBoxLayout(axesBox);

    for ( int i = PlotSettings::AxisBegin; 
        i <= PlotSettings::Logarithmic; i++ ) 
    {
        d_checkBox[i] = new QCheckBox(axesBox);
        axesBoxLayout->addWidget(d_checkBox[i]);
    }

    QGroupBox *gridBox = new QGroupBox("Grids", this);
    QVBoxLayout* gridBoxLayout = new QVBoxLayout(gridBox);
    
    for ( int scaleId = 0; scaleId < QwtPolar::ScaleCount; scaleId++ )
    {
        int idx = PlotSettings::MajorGridBegin + scaleId;
        d_checkBox[idx] = new QCheckBox(gridBox);
        gridBoxLayout->addWidget(d_checkBox[idx]);

        idx = PlotSettings::MinorGridBegin + scaleId;
        d_checkBox[idx] = new QCheckBox(gridBox);
        gridBoxLayout->addWidget(d_checkBox[idx]);
    }
    gridBoxLayout->addStretch(10);

    QGroupBox *otherBox = new QGroupBox("Other", this);
    QVBoxLayout* otherBoxLayout = new QVBoxLayout(otherBox);

    for ( int i = PlotSettings::AxisBegin + QwtPolar::AxesCount;
        i < PlotSettings::NumFlags; i++ )
    {
        d_checkBox[i] = new QCheckBox(otherBox);
        otherBoxLayout->addWidget(d_checkBox[i]);
    }
    otherBoxLayout->addStretch(10);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(axesBox);
    layout->addWidget(gridBox);
    layout->addWidget(otherBox);
    layout->addStretch(10);

    for ( int i = 0; i < PlotSettings::NumFlags; i++ )
    {
        d_checkBox[i]->setText(label(i));
        connect(d_checkBox[i], SIGNAL(clicked()), this, SLOT(edited()) );
    }
}

void SettingsEditor::showSettings(const PlotSettings &settings)
{
    blockSignals(true);
    for ( int i = 0; i < PlotSettings::NumFlags; i++ )
        d_checkBox[i]->setChecked(settings.flags[i]);

    blockSignals(false);
    updateEditor();
}

PlotSettings SettingsEditor::settings() const
{
    PlotSettings s;
    for ( int i = 0; i < PlotSettings::NumFlags; i++ )
        s.flags[i] = d_checkBox[i]->isChecked();
    return s;
}

void SettingsEditor::edited()
{
    updateEditor();

    const PlotSettings s = settings();
    emit edited(s);
}

void SettingsEditor::updateEditor()
{
    for ( int scaleId = 0; scaleId < QwtPolar::ScaleCount; scaleId++ )
    {
        d_checkBox[PlotSettings::MinorGridBegin+scaleId]->setEnabled(
            d_checkBox[PlotSettings::MajorGridBegin+scaleId]->isChecked());
    }
}

QString SettingsEditor::label(int flag) const
{
    switch(flag)
    {
        case PlotSettings::MajorGridBegin + QwtPolar::ScaleAzimuth:
            return "Azimuth";
        case PlotSettings::MajorGridBegin + QwtPolar::ScaleRadius:
            return "Radius";
        case PlotSettings::MinorGridBegin + QwtPolar::ScaleAzimuth:
            return "Azimuth Minor";
        case PlotSettings::MinorGridBegin + QwtPolar::ScaleRadius:
            return "Radius Minor";
        case PlotSettings::AxisBegin + QwtPolar::AxisAzimuth:
            return "Azimuth";
        case PlotSettings::AxisBegin + QwtPolar::AxisLeft:
            return "Left";
        case PlotSettings::AxisBegin + QwtPolar::AxisRight:
            return "Right";
        case PlotSettings::AxisBegin + QwtPolar::AxisTop:
            return "Top";
        case PlotSettings::AxisBegin + QwtPolar::AxisBottom:
            return "Bottom";
        case PlotSettings::AutoScaling: 
            return "Auto Scaling";
        case PlotSettings::Inverted:
            return "Inverted";
        case PlotSettings::Logarithmic:
            return "Logarithmic";
#if QT_VERSION >= 0x040000
        case PlotSettings::Antialiasing:
            return "Antialiasing";
#endif
        case PlotSettings::CurveBegin + PlotSettings::Spiral:
            return "Spiral Curve";
        case PlotSettings::CurveBegin + PlotSettings::Rose:
            return "Rose Curve";
    }
    return QString();
}
