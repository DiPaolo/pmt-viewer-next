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

    //    SendDlgItemMessage(hwndDlg, IDC_PROGRAM_DESCRIPTORS, LB_RESETCONTENT, 0, 0);

    //    for (Descriptors::const_iterator iter = pPMS->program_descriptors.begin();
    //         iter != pPMS->program_descriptors.end(); iter++) {
    //        TCHAR szByte[6] = { 0 };
    //        TCHAR szBytes[256 * 5 + 1] = { 0 }; // maximum 256 1-byte values (" 0xFF")
    //        for (BYTE i = 0; i < iter->length; i++) {
    //            wsprintf(szByte, " 0x%X", iter->pbData[i]);
    //            lstrcat(szBytes, szByte);
    //        }
    //        wsprintf(sz, TEXT("{tag: %u; length: %u; data:%s}"), iter->tag, iter->length, szBytes);
    //        SendDlgItemMessage(hwndDlg, IDC_PROGRAM_DESCRIPTORS, LB_ADDSTRING, 0, (LPARAM)sz);
    //    }

    //    // Fill the tree-view with the values of ES descriptors

    //    HWND hwndTV = GetDlgItem(hwndDlg, IDC_ES_DESCRIPTORS);

    //    TreeView_DeleteAllItems(hwndTV);

    //    for (PMTable::const_iterator iter = pPMS->m_PMT.begin(); iter != pPMS->m_PMT.end(); iter++) {
    //        wsprintf(sz, TEXT("{stream type: %u; elementary PID: %u; ES info length: %u}"),
    //            iter->stream_type, iter->elementary_PID, iter->ES_info_length);

    //        TVITEMEX tvi = { 0 };
    //        tvi.mask = TVIF_TEXT;
    //        tvi.pszText = sz;
    //        tvi.cchTextMax = sizeof(sz) / sizeof(sz[0]);

    //        TVINSERTSTRUCT tvis = { 0 };
    //        tvis.hParent = TVI_ROOT;
    //        tvis.hInsertAfter = TVI_LAST;
    //        tvis.itemex = tvi;

    //        HTREEITEM hTIRoot = TreeView_InsertItem(hwndTV, &tvis);

    //        for (Descriptors::const_iterator descriptorsIter = iter->ES_descriptors.begin();
    //             descriptorsIter != iter->ES_descriptors.end(); descriptorsIter++) {
    //            TCHAR szByte[6] = { 0 };
    //            TCHAR szBytes[256 * 5 + 1] = { 0 }; // maximum 256 1-byte values (" 0xFF")
    //            for (BYTE i = 0; i < descriptorsIter->length; i++) {
    //                wsprintf(szByte, " 0x%X", descriptorsIter->pbData[i]);
    //                lstrcat(szBytes, szByte);
    //            }
    //            wsprintf(sz, TEXT("{tag: %u; length: %u; data:%s}"),
    //                descriptorsIter->tag, descriptorsIter->length, szBytes);

    //            tvis.hParent = hTIRoot;
    //            tvis.hInsertAfter = TVI_LAST;
    //            TreeView_InsertItem(hwndTV, &tvis);
    //        }
    //    }
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
