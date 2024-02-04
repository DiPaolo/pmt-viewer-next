/*******************************************************************************
 * File: PMT Viewer.cpp
 *
 * Description: main file in project.
 *
 * Copyright (c) Ditenbir Pavel, 2007, 2024.
 *
 *******************************************************************************/

#include "main_window.h"
#include "src/ui/ui_main_window.h"
#include <QFileDialog>
#include <QMessageBox>
#include <sstream>

Dialog::Dialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    connect(ui->openFile, &QPushButton::clicked, this, &Dialog::OpenFile);

    connect(ui->showFirst, &QPushButton::clicked, this, [this]() { PMSNavigate(s_TS, first); });
    connect(ui->showPrev, &QPushButton::clicked, this, [this]() { PMSNavigate(s_TS, prev); });
    connect(ui->showNext, &QPushButton::clicked, this, [this]() { PMSNavigate(s_TS, next); });
    connect(ui->showLast, &QPushButton::clicked, this, [this]() { PMSNavigate(s_TS, last); });

    ResetAllControls();
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::OpenFile()
{
    // user pushed "open" button, prepare OPENFILENAME
    // structure and show standard dialog box for choosing file

    QString szFileName = QFileDialog::getOpenFileName(this, "Open Media File", QString(),
        "MPEG files (*.mpg *.mpeg);;"
        "MPEG Transport Stream files (*.ts);;"
        "All files (*.*)");

    if (!szFileName.isEmpty()) {
        s_TS.Close();
        ResetAllControls();

        if (!s_TS.Open(szFileName.toStdString())) {
            // file not opened
            QMessageBox::warning(this, QString(), "File not opened.");
            return;
        }

        if (!s_TS.IsMPEG2TS()) {
            // file is not a MPEG-2 Transport Stream; so close it
            QMessageBox::warning(this, QString(),
                "File is not MPEG-2 Transport Stream or some packets are incorrect. File will be closed.");
            s_TS.Close();
            return;
        }

        if (!s_TS.GetPMSCount()) {
            // there is no PM Sections in file
            QMessageBox::warning(this, QString(),
                "File doesn't contains Program Map Sections and will be closed.");
            s_TS.Close();
            return;
        }

        ui->filename->setText(QString(s_TS.GetFileName().c_str()));

        PMSNavigate(s_TS, first);
    }
}

//
// PMSNavigate
//
// Movement beetween PM Sections in TS and enable or disable appropriate buttons.
void Dialog::PMSNavigate(CTransportStream& TS, Navigation navigation)
{
    static uint32_t s_uCurPMS = 0; // current PMS number

    uint32_t uPMS = 0;
    PM_SECTION PMS;

    switch (navigation) {
    case first:
        if (uint32_t uNum = TS.GetFirstPMSection(&PMS, &uPMS)) {
            s_uCurPMS = 1;
            ShowPMSInfo(&PMS, uPMS, uNum);
        } else
            return;

        break;

    case last:
        if (uint32_t uNum = TS.GetLastPMSection(&PMS, &uPMS)) {
            s_uCurPMS = TS.GetPMSCount();
            ShowPMSInfo(&PMS, uPMS, uNum);
        } else
            return;

        break;

    case prev:
        if (uint32_t uNum = TS.GetPrevPMSection(&PMS, &uPMS)) {
            s_uCurPMS--;
            ShowPMSInfo(&PMS, uPMS, uNum);
        } else
            return;

        break;

    case next:
        if (uint32_t uNum = TS.GetNextPMSection(&PMS, &uPMS)) {
            s_uCurPMS++;
            ShowPMSInfo(&PMS, uPMS, uNum);
        } else
            return;

        break;

    default:
        return;
    }

    bool fBtnFirst = true,
         fBtnPrev = true,
         fBtnNext = true,
         fBtnLast = true;

    if (s_uCurPMS <= 1)
        fBtnFirst = fBtnPrev = false;
    if (s_uCurPMS >= TS.GetPMSCount())
        fBtnNext = fBtnLast = false;

    ui->showFirst->setEnabled(fBtnFirst);
    ui->showPrev->setEnabled(fBtnPrev);
    ui->showNext->setEnabled(fBtnNext);
    ui->showLast->setEnabled(fBtnLast);
}

//
// ShowPMSInfo
//
void Dialog::ShowPMSInfo(const PM_SECTION* pPMS, uint32_t uPMSNum, uint32_t uPacketNum)
{
    std::ostringstream ss;
    ss << "Program Map Section #" << uPMSNum << "(Packet #" << uPacketNum << ")";

    ui->groupBox->setTitle(ss.str().c_str());

    ui->tableId->setNum(pPMS->table_id);
    ui->sectionSyntaxIndicator->setText(pPMS->section_syntax_indicator ? "Yes" : "No");
    ui->sectionLength->setNum(pPMS->section_length);
    ui->programNumber->setNum(pPMS->program_number);
    ui->versionNumber->setNum(pPMS->version_number);
    ui->currentNextIndicator->setText(pPMS->current_next_indicator ? "Yes" : "No");
    ui->sectionNumber->setNum(pPMS->section_number);
    ui->lastSectionNumber->setNum(pPMS->last_section_number);
    ui->pcrPid->setNum(pPMS->PCR_PID);
    ui->programInfoLength->setNum(pPMS->program_info_length);
    ui->crc->setText(QString::number(pPMS->CRC_32));

    // Fill the list box with the values of program descriptors

    ui->programDescriptors->clear();

    for (Descriptors::const_iterator iter = pPMS->program_descriptors.begin(); iter != pPMS->program_descriptors.end(); iter++) {
        std::stringstream bytesSs;
        for (uint8_t i = 0; i < iter->length; i++) {
            bytesSs << "0x" << std::hex << std::uppercase << iter->pbData[i];
        }

        ss << "{tag: " << iter->tag << "; length: " << iter->length << "; data: " << bytesSs.str() << "}";

        ui->programDescriptors->addItem(ss.str().c_str());
    }

    // Fill the tree-view with the values of ES descriptors

    ui->esDescriptors->clear();

    for (PMTable::const_iterator iter = pPMS->m_PMT.begin(); iter != pPMS->m_PMT.end(); iter++) {
        std::stringstream pmsSs;
        pmsSs << "{stream type: " << iter->stream_type << "; elementary PID: " << iter->elementary_PID << "; ES info length: " << iter->ES_info_length << "}";

        auto topLevelItem = new QTreeWidgetItem({ pmsSs.str().c_str() });

        for (Descriptors::const_iterator descriptorsIter = iter->ES_descriptors.begin(); descriptorsIter != iter->ES_descriptors.end(); descriptorsIter++) {
            std::stringstream bytesSs;
            for (uint8_t i = 0; i < descriptorsIter->length; i++) {
                bytesSs << "0x" << std::hex << std::uppercase << descriptorsIter->pbData[i];
            }

            std::stringstream descriptorSs;
            descriptorSs << "{tag: " << descriptorsIter->tag << "; length: " << descriptorsIter->length << "; data: " << bytesSs.str() << "}";

            topLevelItem->addChild(new QTreeWidgetItem({ descriptorSs.str().c_str() }));
        }

        ui->esDescriptors->addTopLevelItem(topLevelItem);
    }
}

//
// ResetAllControls
//
// Reset text in all controls of dialog to it's initial state.
void Dialog::ResetAllControls()
{
    ui->filename->clear();
    ui->groupBox->setTitle("Program Map Section");

    char sz[] = "-";

    ui->tableId->setText(sz);
    ui->sectionSyntaxIndicator->setText(sz);
    ui->sectionLength->setText(sz);
    ui->programNumber->setText(sz);
    ui->versionNumber->setText(sz);
    ui->currentNextIndicator->setText(sz);
    ui->sectionNumber->setText(sz);
    ui->lastSectionNumber->setText(sz);
    ui->pcrPid->setText(sz);
    ui->programInfoLength->setText(sz);
    ui->crc->setText(sz);

    ui->programDescriptors->clear();
    ui->esDescriptors->clear();

    ui->showFirst->setEnabled(false);
    ui->showPrev->setEnabled(false);
    ui->showNext->setEnabled(false);
    ui->showLast->setEnabled(false);
}
