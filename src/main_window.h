/*******************************************************************************
 * File: PMT Viewer.cpp
 *
 * Description: main file in project.
 *
 * Copyright (c) Ditenbir Pavel, 2007, 2024.
 *
 *******************************************************************************/
#pragma once

#include "transport_stream.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class Dialog;
}
QT_END_NAMESPACE

class Dialog : public QDialog {
    Q_OBJECT

    enum Navigation {
        first,
        last,
        prev,
        next
    };

public:
    Dialog(QWidget* parent = nullptr);
    ~Dialog();

private slots:
    void OpenFile();

private:
    void PMSNavigate(CTransportStream& TS, Navigation navigation);
    void ShowPMSInfo(const PM_SECTION* pPMS, uint32_t uPMSNum, uint32_t uPacketNum);
    void ResetAllControls();

private:
    Ui::Dialog* ui;

    CTransportStream s_TS;
};
