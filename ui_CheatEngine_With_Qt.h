/********************************************************************************
** Form generated from reading UI file 'CheatEngine_With_Qt.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CHEATENGINE_WITH_QT_H
#define UI_CHEATENGINE_WITH_QT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CheatEngine_With_QtClass
{
public:
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    QVBoxLayout *verticalLayout_13;
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *verticalLayout_scan_panel;
    QVBoxLayout *verticalLayout_10;
    QVBoxLayout *verticalLayout_9;
    QHBoxLayout *horizontalLayout_9;
    QPushButton *pushButton_openProcess;
    QPushButton *pushButton_save_data_to_file;
    QPushButton *pushButton_load_data_from_file;
    QSpacerItem *horizontalSpacer;
    QVBoxLayout *verticalLayout_2;
    QLabel *label_Process_name;
    QProgressBar *progressBar;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_11;
    QHBoxLayout *horizontalLayout_2;
    QCheckBox *checkBox_Hex_Value;
    QLineEdit *lineEdit_ValueInput;
    QLabel *label_and;
    QLineEdit *lineEdit_ValueInput2;
    QHBoxLayout *horizontalLayout_10;
    QVBoxLayout *verticalLayout_5;
    QComboBox *comboBox_Value_Data_Size;
    QComboBox *comboBox_atribute_For_Find;
    QCheckBox *checkBox_compare_first_scan;
    QVBoxLayout *verticalLayout_12;
    QPushButton *pushButton_new_find;
    QPushButton *pushButton_next_find;
    QPushButton *pushButton_pre_find;
    QHBoxLayout *horizontalLayout_7;
    QVBoxLayout *verticalLayout_4;
    QCheckBox *checkBox_use_UTF16;
    QCheckBox *checkBox_repeat;
    QCheckBox *checkBox_Only_Simple_Value;
    QCheckBox *checkBox_Not;
    QCheckBox *checkBox_Caps_Check;
    QCheckBox *checkBox_Include_Code_Section;
    QCheckBox *checkBox_percent;
    QSpacerItem *verticalSpacer_2;
    QVBoxLayout *verticalLayout_7;
    QLabel *label;
    QComboBox *comboBox_process_module_List;
    QCheckBox *checkBox_8;
    QHBoxLayout *horizontalLayout_4;
    QCheckBox *checkBox_able_to_execute;
    QCheckBox *checkBox_able_to_write;
    QSpacerItem *horizontalSpacer_4;
    QCheckBox *checkBox_scaning_suspend_process;
    QHBoxLayout *horizontalLayout_3;
    QCheckBox *checkBox_fast_scan;
    QLineEdit *lineEdit_fast_scan_value;
    QVBoxLayout *verticalLayout_6;
    QRadioButton *radioButton_align_size;
    QRadioButton *radioButton_end_number;
    QSpacerItem *horizontalSpacer_5;
    QHBoxLayout *horizontalLayout_5;
    QPushButton *pushButton_Memory_View;
    QPushButton *pushButton_settings;
    QPushButton *pushButton_load_dbvm;
    QSpacerItem *verticalSpacer;
    QVBoxLayout *verticalLayout_8;
    QTabWidget *tabWidget;
    QWidget *tab;
    QHBoxLayout *horizontalLayout_6;
    QPushButton *pushButton_add_scan_TabWidget;
    QPushButton *pushButton_remove_scan_TabWidget;
    QPushButton *pushButton_copy_all_scan_address_to_address_list;
    QSpacerItem *horizontalSpacer_3;
    QHBoxLayout *horizontalLayout_8;
    QPushButton *pushButton_delete_all_address;
    QPushButton *pushButton_add_address;
    QSpacerItem *horizontalSpacer_2;
    QTableWidget *tableWidget_addressList;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *CheatEngine_With_QtClass)
    {
        if (CheatEngine_With_QtClass->objectName().isEmpty())
            CheatEngine_With_QtClass->setObjectName("CheatEngine_With_QtClass");
        CheatEngine_With_QtClass->resize(730, 768);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(CheatEngine_With_QtClass->sizePolicy().hasHeightForWidth());
        CheatEngine_With_QtClass->setSizePolicy(sizePolicy);
        CheatEngine_With_QtClass->setAcceptDrops(false);
        centralWidget = new QWidget(CheatEngine_With_QtClass);
        centralWidget->setObjectName("centralWidget");
        verticalLayout = new QVBoxLayout(centralWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout_13 = new QVBoxLayout();
        verticalLayout_13->setSpacing(6);
        verticalLayout_13->setObjectName("verticalLayout_13");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName("horizontalLayout");
        verticalLayout_scan_panel = new QVBoxLayout();
        verticalLayout_scan_panel->setSpacing(6);
        verticalLayout_scan_panel->setObjectName("verticalLayout_scan_panel");
        verticalLayout_10 = new QVBoxLayout();
        verticalLayout_10->setSpacing(6);
        verticalLayout_10->setObjectName("verticalLayout_10");
        verticalLayout_9 = new QVBoxLayout();
        verticalLayout_9->setSpacing(6);
        verticalLayout_9->setObjectName("verticalLayout_9");
        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setSpacing(6);
        horizontalLayout_9->setObjectName("horizontalLayout_9");
        pushButton_openProcess = new QPushButton(centralWidget);
        pushButton_openProcess->setObjectName("pushButton_openProcess");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(pushButton_openProcess->sizePolicy().hasHeightForWidth());
        pushButton_openProcess->setSizePolicy(sizePolicy1);
        pushButton_openProcess->setMinimumSize(QSize(0, 40));

        horizontalLayout_9->addWidget(pushButton_openProcess);

        pushButton_save_data_to_file = new QPushButton(centralWidget);
        pushButton_save_data_to_file->setObjectName("pushButton_save_data_to_file");
        sizePolicy1.setHeightForWidth(pushButton_save_data_to_file->sizePolicy().hasHeightForWidth());
        pushButton_save_data_to_file->setSizePolicy(sizePolicy1);
        pushButton_save_data_to_file->setMinimumSize(QSize(0, 30));

        horizontalLayout_9->addWidget(pushButton_save_data_to_file);

        pushButton_load_data_from_file = new QPushButton(centralWidget);
        pushButton_load_data_from_file->setObjectName("pushButton_load_data_from_file");
        sizePolicy1.setHeightForWidth(pushButton_load_data_from_file->sizePolicy().hasHeightForWidth());
        pushButton_load_data_from_file->setSizePolicy(sizePolicy1);
        pushButton_load_data_from_file->setMinimumSize(QSize(0, 30));

        horizontalLayout_9->addWidget(pushButton_load_data_from_file);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer);


        verticalLayout_9->addLayout(horizontalLayout_9);


        verticalLayout_10->addLayout(verticalLayout_9);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setSizeConstraint(QLayout::SizeConstraint::SetDefaultConstraint);
        verticalLayout_2->setContentsMargins(-1, -1, -1, 0);
        label_Process_name = new QLabel(centralWidget);
        label_Process_name->setObjectName("label_Process_name");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(label_Process_name->sizePolicy().hasHeightForWidth());
        label_Process_name->setSizePolicy(sizePolicy2);
        label_Process_name->setAutoFillBackground(true);
        label_Process_name->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout_2->addWidget(label_Process_name);

        progressBar = new QProgressBar(centralWidget);
        progressBar->setObjectName("progressBar");
        sizePolicy2.setHeightForWidth(progressBar->sizePolicy().hasHeightForWidth());
        progressBar->setSizePolicy(sizePolicy2);
        progressBar->setValue(5);

        verticalLayout_2->addWidget(progressBar);


        verticalLayout_10->addLayout(verticalLayout_2);


        verticalLayout_scan_panel->addLayout(verticalLayout_10);

        groupBox = new QGroupBox(centralWidget);
        groupBox->setObjectName("groupBox");
        QSizePolicy sizePolicy3(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(groupBox->sizePolicy().hasHeightForWidth());
        groupBox->setSizePolicy(sizePolicy3);
        verticalLayout_11 = new QVBoxLayout(groupBox);
        verticalLayout_11->setSpacing(6);
        verticalLayout_11->setContentsMargins(11, 11, 11, 11);
        verticalLayout_11->setObjectName("verticalLayout_11");
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        checkBox_Hex_Value = new QCheckBox(groupBox);
        checkBox_Hex_Value->setObjectName("checkBox_Hex_Value");
        sizePolicy1.setHeightForWidth(checkBox_Hex_Value->sizePolicy().hasHeightForWidth());
        checkBox_Hex_Value->setSizePolicy(sizePolicy1);
        checkBox_Hex_Value->setLayoutDirection(Qt::LayoutDirection::RightToLeft);

        horizontalLayout_2->addWidget(checkBox_Hex_Value);

        lineEdit_ValueInput = new QLineEdit(groupBox);
        lineEdit_ValueInput->setObjectName("lineEdit_ValueInput");
        sizePolicy1.setHeightForWidth(lineEdit_ValueInput->sizePolicy().hasHeightForWidth());
        lineEdit_ValueInput->setSizePolicy(sizePolicy1);

        horizontalLayout_2->addWidget(lineEdit_ValueInput);

        label_and = new QLabel(groupBox);
        label_and->setObjectName("label_and");
        label_and->setEnabled(false);
        sizePolicy1.setHeightForWidth(label_and->sizePolicy().hasHeightForWidth());
        label_and->setSizePolicy(sizePolicy1);
        label_and->setMinimumSize(QSize(0, 0));
        label_and->setMaximumSize(QSize(30, 16777215));

        horizontalLayout_2->addWidget(label_and);

        lineEdit_ValueInput2 = new QLineEdit(groupBox);
        lineEdit_ValueInput2->setObjectName("lineEdit_ValueInput2");
        lineEdit_ValueInput2->setEnabled(false);
        sizePolicy1.setHeightForWidth(lineEdit_ValueInput2->sizePolicy().hasHeightForWidth());
        lineEdit_ValueInput2->setSizePolicy(sizePolicy1);

        horizontalLayout_2->addWidget(lineEdit_ValueInput2);


        verticalLayout_11->addLayout(horizontalLayout_2);

        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setSpacing(6);
        horizontalLayout_10->setObjectName("horizontalLayout_10");
        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setSpacing(6);
        verticalLayout_5->setObjectName("verticalLayout_5");
        comboBox_Value_Data_Size = new QComboBox(groupBox);
        comboBox_Value_Data_Size->addItem(QString());
        comboBox_Value_Data_Size->addItem(QString());
        comboBox_Value_Data_Size->addItem(QString());
        comboBox_Value_Data_Size->addItem(QString());
        comboBox_Value_Data_Size->addItem(QString());
        comboBox_Value_Data_Size->addItem(QString());
        comboBox_Value_Data_Size->addItem(QString());
        comboBox_Value_Data_Size->addItem(QString());
        comboBox_Value_Data_Size->addItem(QString());
        comboBox_Value_Data_Size->setObjectName("comboBox_Value_Data_Size");
        sizePolicy2.setHeightForWidth(comboBox_Value_Data_Size->sizePolicy().hasHeightForWidth());
        comboBox_Value_Data_Size->setSizePolicy(sizePolicy2);

        verticalLayout_5->addWidget(comboBox_Value_Data_Size);

        comboBox_atribute_For_Find = new QComboBox(groupBox);
        comboBox_atribute_For_Find->addItem(QString());
        comboBox_atribute_For_Find->addItem(QString());
        comboBox_atribute_For_Find->addItem(QString());
        comboBox_atribute_For_Find->addItem(QString());
        comboBox_atribute_For_Find->addItem(QString());
        comboBox_atribute_For_Find->setObjectName("comboBox_atribute_For_Find");
        sizePolicy2.setHeightForWidth(comboBox_atribute_For_Find->sizePolicy().hasHeightForWidth());
        comboBox_atribute_For_Find->setSizePolicy(sizePolicy2);

        verticalLayout_5->addWidget(comboBox_atribute_For_Find);

        checkBox_compare_first_scan = new QCheckBox(groupBox);
        checkBox_compare_first_scan->setObjectName("checkBox_compare_first_scan");
        checkBox_compare_first_scan->setEnabled(false);
        sizePolicy1.setHeightForWidth(checkBox_compare_first_scan->sizePolicy().hasHeightForWidth());
        checkBox_compare_first_scan->setSizePolicy(sizePolicy1);

        verticalLayout_5->addWidget(checkBox_compare_first_scan);


        horizontalLayout_10->addLayout(verticalLayout_5);

        verticalLayout_12 = new QVBoxLayout();
        verticalLayout_12->setSpacing(6);
        verticalLayout_12->setObjectName("verticalLayout_12");
        pushButton_new_find = new QPushButton(groupBox);
        pushButton_new_find->setObjectName("pushButton_new_find");
        sizePolicy1.setHeightForWidth(pushButton_new_find->sizePolicy().hasHeightForWidth());
        pushButton_new_find->setSizePolicy(sizePolicy1);
        pushButton_new_find->setMinimumSize(QSize(0, 40));

        verticalLayout_12->addWidget(pushButton_new_find);

        pushButton_next_find = new QPushButton(groupBox);
        pushButton_next_find->setObjectName("pushButton_next_find");
        sizePolicy1.setHeightForWidth(pushButton_next_find->sizePolicy().hasHeightForWidth());
        pushButton_next_find->setSizePolicy(sizePolicy1);

        verticalLayout_12->addWidget(pushButton_next_find);

        pushButton_pre_find = new QPushButton(groupBox);
        pushButton_pre_find->setObjectName("pushButton_pre_find");
        sizePolicy1.setHeightForWidth(pushButton_pre_find->sizePolicy().hasHeightForWidth());
        pushButton_pre_find->setSizePolicy(sizePolicy1);

        verticalLayout_12->addWidget(pushButton_pre_find);


        horizontalLayout_10->addLayout(verticalLayout_12);


        verticalLayout_11->addLayout(horizontalLayout_10);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setSpacing(6);
        horizontalLayout_7->setObjectName("horizontalLayout_7");
        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setSpacing(6);
        verticalLayout_4->setObjectName("verticalLayout_4");
        checkBox_use_UTF16 = new QCheckBox(groupBox);
        checkBox_use_UTF16->setObjectName("checkBox_use_UTF16");
        sizePolicy1.setHeightForWidth(checkBox_use_UTF16->sizePolicy().hasHeightForWidth());
        checkBox_use_UTF16->setSizePolicy(sizePolicy1);
        checkBox_use_UTF16->setMinimumSize(QSize(0, 25));

        verticalLayout_4->addWidget(checkBox_use_UTF16);

        checkBox_repeat = new QCheckBox(groupBox);
        checkBox_repeat->setObjectName("checkBox_repeat");
        sizePolicy1.setHeightForWidth(checkBox_repeat->sizePolicy().hasHeightForWidth());
        checkBox_repeat->setSizePolicy(sizePolicy1);

        verticalLayout_4->addWidget(checkBox_repeat);

        checkBox_Only_Simple_Value = new QCheckBox(groupBox);
        checkBox_Only_Simple_Value->setObjectName("checkBox_Only_Simple_Value");
        sizePolicy1.setHeightForWidth(checkBox_Only_Simple_Value->sizePolicy().hasHeightForWidth());
        checkBox_Only_Simple_Value->setSizePolicy(sizePolicy1);

        verticalLayout_4->addWidget(checkBox_Only_Simple_Value);

        checkBox_Not = new QCheckBox(groupBox);
        checkBox_Not->setObjectName("checkBox_Not");
        sizePolicy1.setHeightForWidth(checkBox_Not->sizePolicy().hasHeightForWidth());
        checkBox_Not->setSizePolicy(sizePolicy1);

        verticalLayout_4->addWidget(checkBox_Not);

        checkBox_Caps_Check = new QCheckBox(groupBox);
        checkBox_Caps_Check->setObjectName("checkBox_Caps_Check");
        sizePolicy1.setHeightForWidth(checkBox_Caps_Check->sizePolicy().hasHeightForWidth());
        checkBox_Caps_Check->setSizePolicy(sizePolicy1);

        verticalLayout_4->addWidget(checkBox_Caps_Check);

        checkBox_Include_Code_Section = new QCheckBox(groupBox);
        checkBox_Include_Code_Section->setObjectName("checkBox_Include_Code_Section");
        sizePolicy1.setHeightForWidth(checkBox_Include_Code_Section->sizePolicy().hasHeightForWidth());
        checkBox_Include_Code_Section->setSizePolicy(sizePolicy1);

        verticalLayout_4->addWidget(checkBox_Include_Code_Section);

        checkBox_percent = new QCheckBox(groupBox);
        checkBox_percent->setObjectName("checkBox_percent");
        sizePolicy1.setHeightForWidth(checkBox_percent->sizePolicy().hasHeightForWidth());
        checkBox_percent->setSizePolicy(sizePolicy1);

        verticalLayout_4->addWidget(checkBox_percent);

        verticalSpacer_2 = new QSpacerItem(20, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_4->addItem(verticalSpacer_2);


        horizontalLayout_7->addLayout(verticalLayout_4);

        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setSpacing(6);
        verticalLayout_7->setObjectName("verticalLayout_7");
        label = new QLabel(groupBox);
        label->setObjectName("label");
        label->setEnabled(true);
        sizePolicy1.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy1);
        label->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);

        verticalLayout_7->addWidget(label);

        comboBox_process_module_List = new QComboBox(groupBox);
        comboBox_process_module_List->addItem(QString());
        comboBox_process_module_List->setObjectName("comboBox_process_module_List");
        sizePolicy2.setHeightForWidth(comboBox_process_module_List->sizePolicy().hasHeightForWidth());
        comboBox_process_module_List->setSizePolicy(sizePolicy2);
        comboBox_process_module_List->setLayoutDirection(Qt::LayoutDirection::LeftToRight);

        verticalLayout_7->addWidget(comboBox_process_module_List);

        checkBox_8 = new QCheckBox(groupBox);
        checkBox_8->setObjectName("checkBox_8");
        sizePolicy1.setHeightForWidth(checkBox_8->sizePolicy().hasHeightForWidth());
        checkBox_8->setSizePolicy(sizePolicy1);

        verticalLayout_7->addWidget(checkBox_8);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        checkBox_able_to_execute = new QCheckBox(groupBox);
        checkBox_able_to_execute->setObjectName("checkBox_able_to_execute");
        sizePolicy1.setHeightForWidth(checkBox_able_to_execute->sizePolicy().hasHeightForWidth());
        checkBox_able_to_execute->setSizePolicy(sizePolicy1);

        horizontalLayout_4->addWidget(checkBox_able_to_execute);

        checkBox_able_to_write = new QCheckBox(groupBox);
        checkBox_able_to_write->setObjectName("checkBox_able_to_write");
        sizePolicy1.setHeightForWidth(checkBox_able_to_write->sizePolicy().hasHeightForWidth());
        checkBox_able_to_write->setSizePolicy(sizePolicy1);

        horizontalLayout_4->addWidget(checkBox_able_to_write);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_4);


        verticalLayout_7->addLayout(horizontalLayout_4);

        checkBox_scaning_suspend_process = new QCheckBox(groupBox);
        checkBox_scaning_suspend_process->setObjectName("checkBox_scaning_suspend_process");
        sizePolicy1.setHeightForWidth(checkBox_scaning_suspend_process->sizePolicy().hasHeightForWidth());
        checkBox_scaning_suspend_process->setSizePolicy(sizePolicy1);

        verticalLayout_7->addWidget(checkBox_scaning_suspend_process);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        checkBox_fast_scan = new QCheckBox(groupBox);
        checkBox_fast_scan->setObjectName("checkBox_fast_scan");
        sizePolicy1.setHeightForWidth(checkBox_fast_scan->sizePolicy().hasHeightForWidth());
        checkBox_fast_scan->setSizePolicy(sizePolicy1);

        horizontalLayout_3->addWidget(checkBox_fast_scan);

        lineEdit_fast_scan_value = new QLineEdit(groupBox);
        lineEdit_fast_scan_value->setObjectName("lineEdit_fast_scan_value");
        QSizePolicy sizePolicy4(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Expanding);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(lineEdit_fast_scan_value->sizePolicy().hasHeightForWidth());
        lineEdit_fast_scan_value->setSizePolicy(sizePolicy4);
        lineEdit_fast_scan_value->setMaximumSize(QSize(20, 16777215));
        lineEdit_fast_scan_value->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);

        horizontalLayout_3->addWidget(lineEdit_fast_scan_value);

        verticalLayout_6 = new QVBoxLayout();
        verticalLayout_6->setSpacing(6);
        verticalLayout_6->setObjectName("verticalLayout_6");
        radioButton_align_size = new QRadioButton(groupBox);
        radioButton_align_size->setObjectName("radioButton_align_size");
        sizePolicy1.setHeightForWidth(radioButton_align_size->sizePolicy().hasHeightForWidth());
        radioButton_align_size->setSizePolicy(sizePolicy1);
        radioButton_align_size->setChecked(true);

        verticalLayout_6->addWidget(radioButton_align_size);

        radioButton_end_number = new QRadioButton(groupBox);
        radioButton_end_number->setObjectName("radioButton_end_number");
        sizePolicy1.setHeightForWidth(radioButton_end_number->sizePolicy().hasHeightForWidth());
        radioButton_end_number->setSizePolicy(sizePolicy1);

        verticalLayout_6->addWidget(radioButton_end_number);


        horizontalLayout_3->addLayout(verticalLayout_6);

        horizontalSpacer_5 = new QSpacerItem(40, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_5);


        verticalLayout_7->addLayout(horizontalLayout_3);


        horizontalLayout_7->addLayout(verticalLayout_7);


        verticalLayout_11->addLayout(horizontalLayout_7);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setSpacing(6);
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        pushButton_Memory_View = new QPushButton(groupBox);
        pushButton_Memory_View->setObjectName("pushButton_Memory_View");
        sizePolicy1.setHeightForWidth(pushButton_Memory_View->sizePolicy().hasHeightForWidth());
        pushButton_Memory_View->setSizePolicy(sizePolicy1);
        pushButton_Memory_View->setMinimumSize(QSize(0, 40));

        horizontalLayout_5->addWidget(pushButton_Memory_View);

        pushButton_settings = new QPushButton(groupBox);
        pushButton_settings->setObjectName("pushButton_settings");
        sizePolicy1.setHeightForWidth(pushButton_settings->sizePolicy().hasHeightForWidth());
        pushButton_settings->setSizePolicy(sizePolicy1);
        pushButton_settings->setMinimumSize(QSize(0, 40));

        horizontalLayout_5->addWidget(pushButton_settings);

        pushButton_load_dbvm = new QPushButton(groupBox);
        pushButton_load_dbvm->setObjectName("pushButton_load_dbvm");
        sizePolicy1.setHeightForWidth(pushButton_load_dbvm->sizePolicy().hasHeightForWidth());
        pushButton_load_dbvm->setSizePolicy(sizePolicy1);
        pushButton_load_dbvm->setMinimumSize(QSize(0, 40));

        horizontalLayout_5->addWidget(pushButton_load_dbvm);


        verticalLayout_11->addLayout(horizontalLayout_5);


        verticalLayout_scan_panel->addWidget(groupBox);

        verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_scan_panel->addItem(verticalSpacer);


        horizontalLayout->addLayout(verticalLayout_scan_panel);

        verticalLayout_8 = new QVBoxLayout();
        verticalLayout_8->setSpacing(6);
        verticalLayout_8->setObjectName("verticalLayout_8");
        tabWidget = new QTabWidget(centralWidget);
        tabWidget->setObjectName("tabWidget");
        QSizePolicy sizePolicy5(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(10);
        sizePolicy5.setHeightForWidth(tabWidget->sizePolicy().hasHeightForWidth());
        tabWidget->setSizePolicy(sizePolicy5);
        tab = new QWidget();
        tab->setObjectName("tab");
        tabWidget->addTab(tab, QString());

        verticalLayout_8->addWidget(tabWidget);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setSpacing(6);
        horizontalLayout_6->setObjectName("horizontalLayout_6");
        pushButton_add_scan_TabWidget = new QPushButton(centralWidget);
        pushButton_add_scan_TabWidget->setObjectName("pushButton_add_scan_TabWidget");
        sizePolicy1.setHeightForWidth(pushButton_add_scan_TabWidget->sizePolicy().hasHeightForWidth());
        pushButton_add_scan_TabWidget->setSizePolicy(sizePolicy1);
        pushButton_add_scan_TabWidget->setMaximumSize(QSize(80, 25));

        horizontalLayout_6->addWidget(pushButton_add_scan_TabWidget);

        pushButton_remove_scan_TabWidget = new QPushButton(centralWidget);
        pushButton_remove_scan_TabWidget->setObjectName("pushButton_remove_scan_TabWidget");
        sizePolicy1.setHeightForWidth(pushButton_remove_scan_TabWidget->sizePolicy().hasHeightForWidth());
        pushButton_remove_scan_TabWidget->setSizePolicy(sizePolicy1);
        pushButton_remove_scan_TabWidget->setMaximumSize(QSize(80, 25));

        horizontalLayout_6->addWidget(pushButton_remove_scan_TabWidget);

        pushButton_copy_all_scan_address_to_address_list = new QPushButton(centralWidget);
        pushButton_copy_all_scan_address_to_address_list->setObjectName("pushButton_copy_all_scan_address_to_address_list");
        sizePolicy1.setHeightForWidth(pushButton_copy_all_scan_address_to_address_list->sizePolicy().hasHeightForWidth());
        pushButton_copy_all_scan_address_to_address_list->setSizePolicy(sizePolicy1);

        horizontalLayout_6->addWidget(pushButton_copy_all_scan_address_to_address_list);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_6->addItem(horizontalSpacer_3);


        verticalLayout_8->addLayout(horizontalLayout_6);


        horizontalLayout->addLayout(verticalLayout_8);

        horizontalLayout->setStretch(1, 1);

        verticalLayout_13->addLayout(horizontalLayout);


        verticalLayout->addLayout(verticalLayout_13);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setSpacing(6);
        horizontalLayout_8->setObjectName("horizontalLayout_8");
        pushButton_delete_all_address = new QPushButton(centralWidget);
        pushButton_delete_all_address->setObjectName("pushButton_delete_all_address");
        sizePolicy1.setHeightForWidth(pushButton_delete_all_address->sizePolicy().hasHeightForWidth());
        pushButton_delete_all_address->setSizePolicy(sizePolicy1);

        horizontalLayout_8->addWidget(pushButton_delete_all_address);

        pushButton_add_address = new QPushButton(centralWidget);
        pushButton_add_address->setObjectName("pushButton_add_address");
        sizePolicy1.setHeightForWidth(pushButton_add_address->sizePolicy().hasHeightForWidth());
        pushButton_add_address->setSizePolicy(sizePolicy1);

        horizontalLayout_8->addWidget(pushButton_add_address);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_8->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout_8);

        tableWidget_addressList = new QTableWidget(centralWidget);
        tableWidget_addressList->setObjectName("tableWidget_addressList");
        tableWidget_addressList->setRowCount(0);
        tableWidget_addressList->setColumnCount(0);

        verticalLayout->addWidget(tableWidget_addressList);

        CheatEngine_With_QtClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(CheatEngine_With_QtClass);
        statusBar->setObjectName("statusBar");
        CheatEngine_With_QtClass->setStatusBar(statusBar);

        retranslateUi(CheatEngine_With_QtClass);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(CheatEngine_With_QtClass);
    } // setupUi

    void retranslateUi(QMainWindow *CheatEngine_With_QtClass)
    {
        CheatEngine_With_QtClass->setWindowTitle(QCoreApplication::translate("CheatEngine_With_QtClass", "CheatEngine_With_Qt", nullptr));
        pushButton_openProcess->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\346\211\223\345\274\200\350\277\233\347\250\213", nullptr));
        pushButton_save_data_to_file->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\344\277\235\345\255\230\346\225\260\346\215\256", nullptr));
        pushButton_load_data_from_file->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\212\240\350\275\275\346\225\260\346\215\256", nullptr));
        label_Process_name->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\350\257\267\351\200\211\346\213\251\350\277\233\347\250\213", nullptr));
        groupBox->setTitle(QString());
        checkBox_Hex_Value->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "Hex", nullptr));
        label_and->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "and", nullptr));
        comboBox_Value_Data_Size->setItemText(0, QCoreApplication::translate("CheatEngine_With_QtClass", "1\345\255\227\350\212\202", nullptr));
        comboBox_Value_Data_Size->setItemText(1, QCoreApplication::translate("CheatEngine_With_QtClass", "2\345\255\227\350\212\202", nullptr));
        comboBox_Value_Data_Size->setItemText(2, QCoreApplication::translate("CheatEngine_With_QtClass", "4\345\255\227\350\212\202", nullptr));
        comboBox_Value_Data_Size->setItemText(3, QCoreApplication::translate("CheatEngine_With_QtClass", "8\345\255\227\350\212\202", nullptr));
        comboBox_Value_Data_Size->setItemText(4, QCoreApplication::translate("CheatEngine_With_QtClass", "\345\215\225\346\265\256\347\202\271", nullptr));
        comboBox_Value_Data_Size->setItemText(5, QCoreApplication::translate("CheatEngine_With_QtClass", "\345\217\214\346\265\256\347\202\271", nullptr));
        comboBox_Value_Data_Size->setItemText(6, QCoreApplication::translate("CheatEngine_With_QtClass", "\345\255\227\347\254\246\344\270\262", nullptr));
        comboBox_Value_Data_Size->setItemText(7, QCoreApplication::translate("CheatEngine_With_QtClass", "\345\255\227\350\212\202\346\225\260\347\273\204", nullptr));
        comboBox_Value_Data_Size->setItemText(8, QCoreApplication::translate("CheatEngine_With_QtClass", "\346\211\200\346\234\211\347\261\273\345\236\213", nullptr));

        comboBox_atribute_For_Find->setItemText(0, QCoreApplication::translate("CheatEngine_With_QtClass", "\347\262\276\347\241\256\346\225\260\345\200\274", nullptr));
        comboBox_atribute_For_Find->setItemText(1, QCoreApplication::translate("CheatEngine_With_QtClass", "\345\200\274\345\244\247\344\272\216", nullptr));
        comboBox_atribute_For_Find->setItemText(2, QCoreApplication::translate("CheatEngine_With_QtClass", "\345\200\274\345\260\217\344\272\216", nullptr));
        comboBox_atribute_For_Find->setItemText(3, QCoreApplication::translate("CheatEngine_With_QtClass", "\344\273\213\344\272\216\344\270\244\350\200\205\344\271\213\351\227\264\347\232\204\345\200\274", nullptr));
        comboBox_atribute_For_Find->setItemText(4, QCoreApplication::translate("CheatEngine_With_QtClass", "\346\234\252\347\237\245\347\232\204\345\210\235\345\247\213\345\200\274", nullptr));

        checkBox_compare_first_scan->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\257\271\346\257\224\351\246\226\346\254\241\346\237\245\346\211\276", nullptr));
        pushButton_new_find->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\346\226\260\346\237\245\346\211\276", nullptr));
        pushButton_next_find->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\206\215\346\254\241\346\237\245\346\211\276", nullptr));
        pushButton_pre_find->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\344\270\212\344\270\200\346\254\241\346\237\245\346\211\276", nullptr));
        checkBox_use_UTF16->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "UTF-16\345\256\275\345\255\227\350\212\202", nullptr));
        checkBox_repeat->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\276\252\347\216\257\351\207\215\345\244\215\346\220\234\347\264\242", nullptr));
        checkBox_Only_Simple_Value->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\344\273\205\347\256\200\345\215\225\345\200\274", nullptr));
        checkBox_Not->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\351\235\236", nullptr));
        checkBox_Caps_Check->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\214\272\345\210\206\345\244\247\345\260\217\345\206\231", nullptr));
        checkBox_Include_Code_Section->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\220\253\344\273\243\347\240\201\346\256\265\351\203\250\345\210\206", nullptr));
        checkBox_percent->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\350\207\263\345\260\221\347\231\276\345\210\206\344\271\213", nullptr));
        label->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\206\205\345\255\230\346\211\253\346\217\217\351\200\211\351\241\271", nullptr));
        comboBox_process_module_List->setItemText(0, QCoreApplication::translate("CheatEngine_With_QtClass", "All", nullptr));

        checkBox_8->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\206\231\346\227\266\345\206\215\347\224\263\350\257\267\347\211\251\347\220\206\351\241\265\347\232\204\345\206\205\345\255\230", nullptr));
        checkBox_able_to_execute->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\217\257\346\211\247\350\241\214", nullptr));
        checkBox_able_to_write->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\217\257\345\206\231", nullptr));
        checkBox_scaning_suspend_process->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\346\220\234\347\264\242\346\227\266\346\232\202\345\201\234\350\277\233\347\250\213", nullptr));
        checkBox_fast_scan->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\277\253\351\200\237\346\211\253\346\217\217", nullptr));
        lineEdit_fast_scan_value->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "4", nullptr));
        radioButton_align_size->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\257\271\351\275\220\345\244\247\345\260\217", nullptr));
        radioButton_end_number->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\347\273\223\345\260\276\346\225\260\345\255\227", nullptr));
        pushButton_Memory_View->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\346\237\245\347\234\213\345\206\205\345\255\230", nullptr));
        pushButton_settings->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\350\256\276\347\275\256", nullptr));
        pushButton_load_dbvm->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\212\240\350\275\275dbvm", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("CheatEngine_With_QtClass", "\346\237\245\346\211\2761", nullptr));
        pushButton_add_scan_TabWidget->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\346\267\273\345\212\240\346\237\245\346\211\276\350\241\250", nullptr));
        pushButton_remove_scan_TabWidget->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\210\240\351\231\244\346\237\245\346\211\276\350\241\250", nullptr));
        pushButton_copy_all_scan_address_to_address_list->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\244\215\345\210\266\346\211\200\346\234\211\351\200\211\344\270\255\345\234\260\345\235\200\345\210\260\345\234\260\345\235\200\350\241\250", nullptr));
        pushButton_delete_all_address->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\345\210\240\351\231\244\345\210\227\350\241\250\344\270\255\346\211\200\346\234\211\345\234\260\345\235\200", nullptr));
        pushButton_add_address->setText(QCoreApplication::translate("CheatEngine_With_QtClass", "\346\211\213\345\212\250\346\267\273\345\212\240\345\234\260\345\235\200", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CheatEngine_With_QtClass: public Ui_CheatEngine_With_QtClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CHEATENGINE_WITH_QT_H
