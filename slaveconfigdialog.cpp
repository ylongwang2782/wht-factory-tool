#include "slaveconfigdialog.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHeaderView>

QString SlaveConfigData::toJsonString() const
{
    QJsonObject jsonObj;
    jsonObj["name"] = name;
    jsonObj["slaveNum"] = static_cast<int>(config.slaveNum);
    
    QJsonArray slavesArray;
    for (const auto& slave : config.slaves) {
        QJsonObject slaveObj;
        slaveObj["id"] = static_cast<qint64>(slave.id);
        slaveObj["conductionNum"] = static_cast<int>(slave.conductionNum);
        slaveObj["resistanceNum"] = static_cast<int>(slave.resistanceNum);
        slaveObj["clipMode"] = static_cast<int>(slave.clipMode);
        slaveObj["clipStatus"] = static_cast<int>(slave.clipStatus);
        slavesArray.append(slaveObj);
    }
    jsonObj["slaves"] = slavesArray;
    
    QJsonDocument doc(jsonObj);
    return doc.toJson(QJsonDocument::Compact);
}

bool SlaveConfigData::fromJsonString(const QString& jsonStr)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &error);
    
    if (error.error != QJsonParseError::NoError) {
        return false;
    }
    
    QJsonObject jsonObj = doc.object();
    name = jsonObj["name"].toString();
    config.slaveNum = static_cast<uint8_t>(jsonObj["slaveNum"].toInt());
    
    config.slaves.clear();
    QJsonArray slavesArray = jsonObj["slaves"].toArray();
    for (const auto& value : slavesArray) {
        QJsonObject slaveObj = value.toObject();
        WhtsProtocol::Backend2Master::SlaveConfigMessage::SlaveInfo slave;
        slave.id = static_cast<uint32_t>(slaveObj["id"].toInteger());
        slave.conductionNum = static_cast<uint8_t>(slaveObj["conductionNum"].toInt());
        slave.resistanceNum = static_cast<uint8_t>(slaveObj["resistanceNum"].toInt());
        slave.clipMode = static_cast<uint8_t>(slaveObj["clipMode"].toInt());
        slave.clipStatus = static_cast<uint16_t>(slaveObj["clipStatus"].toInt());
        config.slaves.push_back(slave);
    }
    
    return true;
}

QString SlaveConfigData::getConfigDescription() const
{
    return QString("从机数量: %1, 配置项: %2个")
            .arg(config.slaveNum)
            .arg(config.slaves.size());
}

SlaveConfigDialog::SlaveConfigDialog(QWidget *parent)
    : QDialog(parent)
    , m_pLineEditConfigName(nullptr)
    , m_pTableWidgetSlaves(nullptr)
    , m_pPushButtonAddSlave(nullptr)
    , m_pPushButtonRemoveSlave(nullptr)
    , m_pPushButtonOk(nullptr)
    , m_pPushButtonCancel(nullptr)
    , m_bEditMode(false)
{
    InitializeUI();
    setWindowTitle("新建从机配置");
}

SlaveConfigDialog::SlaveConfigDialog(const SlaveConfigData& configData, QWidget *parent)
    : QDialog(parent)
    , m_pLineEditConfigName(nullptr)
    , m_pTableWidgetSlaves(nullptr)
    , m_pPushButtonAddSlave(nullptr)
    , m_pPushButtonRemoveSlave(nullptr)
    , m_pPushButtonOk(nullptr)
    , m_pPushButtonCancel(nullptr)
    , m_configData(configData)
    , m_bEditMode(true)
{
    InitializeUI();
    setWindowTitle("编辑从机配置");
    setConfigData(configData);
}

void SlaveConfigDialog::InitializeUI()
{
    setModal(true);
    resize(600, 400);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 配置名称输入
    QGroupBox* nameGroup = new QGroupBox("配置信息", this);
    QFormLayout* nameLayout = new QFormLayout(nameGroup);
    
    m_pLineEditConfigName = new QLineEdit(this);
    m_pLineEditConfigName->setPlaceholderText("请输入配置名称");
    nameLayout->addRow("配置名称:", m_pLineEditConfigName);
    
    mainLayout->addWidget(nameGroup);
    
    // 从机配置表格
    QGroupBox* slaveGroup = new QGroupBox("从机配置列表", this);
    QVBoxLayout* slaveLayout = new QVBoxLayout(slaveGroup);
    
    // 操作按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_pPushButtonAddSlave = new QPushButton("添加从机", this);
    m_pPushButtonRemoveSlave = new QPushButton("删除从机", this);
    
    buttonLayout->addWidget(m_pPushButtonAddSlave);
    buttonLayout->addWidget(m_pPushButtonRemoveSlave);
    buttonLayout->addStretch();
    
    slaveLayout->addLayout(buttonLayout);
    
    // 从机表格
    m_pTableWidgetSlaves = new QTableWidget(this);
    m_pTableWidgetSlaves->setColumnCount(5);
    QStringList headers;
    headers << "从机ID" << "导通数量" << "阻抗数量" << "卡钉模式" << "卡钉状态";
    m_pTableWidgetSlaves->setHorizontalHeaderLabels(headers);
    m_pTableWidgetSlaves->horizontalHeader()->setStretchLastSection(true);
    m_pTableWidgetSlaves->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pTableWidgetSlaves->setAlternatingRowColors(true);
    
    slaveLayout->addWidget(m_pTableWidgetSlaves);
    mainLayout->addWidget(slaveGroup);
    
    // 底部按钮
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    
    m_pPushButtonOk = new QPushButton("确定", this);
    m_pPushButtonCancel = new QPushButton("取消", this);
    
    bottomLayout->addWidget(m_pPushButtonOk);
    bottomLayout->addWidget(m_pPushButtonCancel);
    
    mainLayout->addLayout(bottomLayout);
    
    // 连接信号和槽
    connect(m_pPushButtonAddSlave, &QPushButton::clicked, this, &SlaveConfigDialog::OnAddSlaveClicked);
    connect(m_pPushButtonRemoveSlave, &QPushButton::clicked, this, &SlaveConfigDialog::OnRemoveSlaveClicked);
    connect(m_pPushButtonOk, &QPushButton::clicked, this, &SlaveConfigDialog::OnOkClicked);
    connect(m_pPushButtonCancel, &QPushButton::clicked, this, &SlaveConfigDialog::OnCancelClicked);
}

void SlaveConfigDialog::OnAddSlaveClicked()
{
    WhtsProtocol::Backend2Master::SlaveConfigMessage::SlaveInfo newSlave;
    newSlave.id = 0x00000001; // 默认ID
    newSlave.conductionNum = 1;
    newSlave.resistanceNum = 1;
    newSlave.clipMode = 0;
    newSlave.clipStatus = 0;
    
    m_configData.config.slaves.push_back(newSlave);
    m_configData.config.slaveNum = static_cast<uint8_t>(m_configData.config.slaves.size());
    
    UpdateSlaveTable();
}

void SlaveConfigDialog::OnRemoveSlaveClicked()
{
    int currentRow = m_pTableWidgetSlaves->currentRow();
    if (currentRow >= 0 && currentRow < static_cast<int>(m_configData.config.slaves.size())) {
        m_configData.config.slaves.erase(m_configData.config.slaves.begin() + currentRow);
        m_configData.config.slaveNum = static_cast<uint8_t>(m_configData.config.slaves.size());
        UpdateSlaveTable();
    }
}

void SlaveConfigDialog::OnOkClicked()
{
    // 验证输入
    if (m_pLineEditConfigName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入配置名称");
        return;
    }
    
    if (m_configData.config.slaves.empty()) {
        QMessageBox::warning(this, "警告", "请至少添加一个从机配置");
        return;
    }
    
    // 从表格中读取数据
    for (int i = 0; i < m_pTableWidgetSlaves->rowCount(); ++i) {
        if (i < static_cast<int>(m_configData.config.slaves.size())) {
            auto& slave = m_configData.config.slaves[i];
            
            bool ok;
            uint32_t id = m_pTableWidgetSlaves->item(i, 0)->text().toUInt(&ok, 16);
            if (ok) slave.id = id;
            
            slave.conductionNum = static_cast<uint8_t>(m_pTableWidgetSlaves->item(i, 1)->text().toUInt());
            slave.resistanceNum = static_cast<uint8_t>(m_pTableWidgetSlaves->item(i, 2)->text().toUInt());
            slave.clipMode = static_cast<uint8_t>(m_pTableWidgetSlaves->item(i, 3)->text().toUInt());
            slave.clipStatus = static_cast<uint16_t>(m_pTableWidgetSlaves->item(i, 4)->text().toUInt());
        }
    }
    
    m_configData.name = m_pLineEditConfigName->text().trimmed();
    m_configData.config.slaveNum = static_cast<uint8_t>(m_configData.config.slaves.size());
    
    accept();
}

void SlaveConfigDialog::OnCancelClicked()
{
    reject();
}

void SlaveConfigDialog::UpdateSlaveTable()
{
    m_pTableWidgetSlaves->setRowCount(static_cast<int>(m_configData.config.slaves.size()));
    
    for (size_t i = 0; i < m_configData.config.slaves.size(); ++i) {
        const auto& slave = m_configData.config.slaves[i];
        
        m_pTableWidgetSlaves->setItem(i, 0, new QTableWidgetItem(QString("0x%1").arg(slave.id, 8, 16, QChar('0')).toUpper()));
        m_pTableWidgetSlaves->setItem(i, 1, new QTableWidgetItem(QString::number(slave.conductionNum)));
        m_pTableWidgetSlaves->setItem(i, 2, new QTableWidgetItem(QString::number(slave.resistanceNum)));
        m_pTableWidgetSlaves->setItem(i, 3, new QTableWidgetItem(QString::number(slave.clipMode)));
        m_pTableWidgetSlaves->setItem(i, 4, new QTableWidgetItem(QString::number(slave.clipStatus)));
    }
}

SlaveConfigData SlaveConfigDialog::getConfigData() const
{
    return m_configData;
}

void SlaveConfigDialog::setConfigData(const SlaveConfigData& configData)
{
    m_configData = configData;
    m_pLineEditConfigName->setText(configData.name);
    UpdateSlaveTable();
}
