#ifndef SLAVECONFIGDIALOG_H
#define SLAVECONFIGDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include "protocol/messages/Backend2Master.h"

struct SlaveConfigData {
    QString name;
    WhtsProtocol::Backend2Master::SlaveConfigMessage config;
    
    // 序列化到JSON字符串（用于保存）
    QString toJsonString() const;
    // 从JSON字符串反序列化
    bool fromJsonString(const QString& jsonStr);
    
    // 获取配置信息的可读描述
    QString getConfigDescription() const;
};

class SlaveConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SlaveConfigDialog(QWidget *parent = nullptr);
    explicit SlaveConfigDialog(const SlaveConfigData& configData, QWidget *parent = nullptr);
    
    SlaveConfigData getConfigData() const;
    void setConfigData(const SlaveConfigData& configData);

private slots:
    void OnAddSlaveClicked();
    void OnRemoveSlaveClicked();
    void OnOkClicked();
    void OnCancelClicked();

private:
    void InitializeUI();
    void UpdateSlaveTable();
    void ValidateInput();
    
private:
    QLineEdit* m_pLineEditConfigName;
    QTableWidget* m_pTableWidgetSlaves;
    QPushButton* m_pPushButtonAddSlave;
    QPushButton* m_pPushButtonRemoveSlave;
    QPushButton* m_pPushButtonOk;
    QPushButton* m_pPushButtonCancel;
    
    SlaveConfigData m_configData;
    bool m_bEditMode;
};

#endif // SLAVECONFIGDIALOG_H
