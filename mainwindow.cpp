#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_pUdpSocket(nullptr)
    , m_localPort(8080)
    , m_remotePort(8081)
    , m_bConnected(false)
    , m_pLogFile(nullptr)
    , m_pLogStream(nullptr)
    , m_pProtocolProcessor(nullptr)
    , m_pSettings(nullptr)
    , m_bDataViewRunning(false)
{
    ui->setupUi(this);
    InitializeUI();
    
    // 创建协议处理器
    m_pProtocolProcessor = new WhtsProtocol::ProtocolProcessor();
    
    // 创建设置对象
    m_pSettings = new QSettings("WHT", "FactoryTool", this);
    
    // 创建日志文件
    QString logFileName = QString("udp_debug_%1.log").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    m_pLogFile = new QFile(logFileName, this);
    if (m_pLogFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_pLogStream = new QTextStream(m_pLogFile);
    }
    
    // 加载从机配置
    LoadSlaveConfigs();
    
    LogMessage("WHT工厂工具启动");
}

MainWindow::~MainWindow()
{
    if (m_pUdpSocket) {
        m_pUdpSocket->close();
        delete m_pUdpSocket;
    }
    
    if (m_pProtocolProcessor) {
        delete m_pProtocolProcessor;
    }
    
    if (m_pLogStream) {
        delete m_pLogStream;
    }
    
    if (m_pLogFile) {
        m_pLogFile->close();
    }
    
    delete ui;
}

void MainWindow::InitializeUI()
{
    // 连接信号和槽
    connect(ui->pushButtonConnect, &QPushButton::clicked, this, &MainWindow::OnConnectClicked);
    connect(ui->pushButtonDisconnect, &QPushButton::clicked, this, &MainWindow::OnDisconnectClicked);
    connect(ui->pushButtonSend, &QPushButton::clicked, this, &MainWindow::OnSendClicked);
    connect(ui->pushButtonClearSend, &QPushButton::clicked, this, &MainWindow::OnClearSendClicked);
    connect(ui->pushButtonClearLog, &QPushButton::clicked, this, &MainWindow::OnClearLogClicked);
    connect(ui->pushButtonQueryDevices, &QPushButton::clicked, this, &MainWindow::OnQueryDevicesClicked);
    connect(ui->pushButtonAddSlaveConfig, &QPushButton::clicked, this, &MainWindow::OnAddSlaveConfigClicked);
    connect(ui->pushButtonStart, &QPushButton::clicked, this, &MainWindow::OnStartClicked);
    connect(ui->pushButtonStop, &QPushButton::clicked, this, &MainWindow::OnStopClicked);
    connect(ui->pushButtonClearData, &QPushButton::clicked, this, &MainWindow::OnClearDataClicked);
    
    // 设置发送框回车键发送
    connect(ui->lineEditSendData, &QLineEdit::returnPressed, this, &MainWindow::OnSendClicked);
    
    // 初始化设备表格
    ui->tableWidgetDevices->setColumnWidth(0, 100); // 设备ID
    ui->tableWidgetDevices->setColumnWidth(1, 80);  // 短ID
    ui->tableWidgetDevices->setColumnWidth(2, 100); // 在线状态
    ui->tableWidgetDevices->setColumnWidth(3, 120); // 版本
    ui->tableWidgetDevices->setColumnWidth(4, 150); // 电池电量
    
    // 初始化从机配置表格
    ui->tableWidgetSlaveConfigs->setColumnWidth(0, 150); // 配置名称
    ui->tableWidgetSlaveConfigs->setColumnWidth(1, 200); // 配置信息
    ui->tableWidgetSlaveConfigs->setColumnWidth(2, 250); // 操作（增加宽度以容纳4个按钮）
    
    // 初始化数据查看表格
    ui->tableWidgetDataView->setColumnWidth(0, 80);  // Slave ID
    ui->tableWidgetDataView->setColumnWidth(1, 40);  // CS
    ui->tableWidgetDataView->setColumnWidth(2, 40);  // SL
    ui->tableWidgetDataView->setColumnWidth(3, 40);  // EUB
    ui->tableWidgetDataView->setColumnWidth(4, 40);  // BLA
    ui->tableWidgetDataView->setColumnWidth(5, 40);  // PS
    ui->tableWidgetDataView->setColumnWidth(6, 40);  // EL1
    ui->tableWidgetDataView->setColumnWidth(7, 40);  // EL2
    ui->tableWidgetDataView->setColumnWidth(8, 40);  // A1
    ui->tableWidgetDataView->setColumnWidth(9, 40);  // A2
    ui->tableWidgetDataView->setColumnWidth(10, 200); // 导通数据
    
    // 设置窗口大小
    resize(1000, 700);
}

void MainWindow::OnConnectClicked()
{
    if (m_pUdpSocket) {
        delete m_pUdpSocket;
        m_pUdpSocket = nullptr;
    }
    
    // 获取配置参数
    m_localAddress = QHostAddress(ui->lineEditLocalIP->text());
    m_localPort = ui->lineEditLocalPort->text().toUShort();
    m_remoteAddress = QHostAddress(ui->lineEditRemoteIP->text());
    m_remotePort = ui->lineEditRemotePort->text().toUShort();
    
    // 创建UDP socket
    m_pUdpSocket = new QUdpSocket(this);
    
    // 连接信号
    connect(m_pUdpSocket, &QUdpSocket::readyRead, this, &MainWindow::OnReadyRead);
    connect(m_pUdpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &MainWindow::OnSocketError);
    
    // 绑定本地地址和端口
    if (m_pUdpSocket->bind(m_localAddress, m_localPort)) {
        m_bConnected = true;
        UpdateConnectionState(true);
        LogMessage(QString("UDP连接成功 - 本地: %1:%2, 目标: %3:%4")
                  .arg(m_localAddress.toString())
                  .arg(m_localPort)
                  .arg(m_remoteAddress.toString())
                  .arg(m_remotePort));
    } else {
        LogMessage(QString("UDP连接失败: %1").arg(m_pUdpSocket->errorString()), "ERROR");
        delete m_pUdpSocket;
        m_pUdpSocket = nullptr;
        m_bConnected = false;
        UpdateConnectionState(false);
    }
}

void MainWindow::OnDisconnectClicked()
{
    if (m_pUdpSocket) {
        m_pUdpSocket->close();
        delete m_pUdpSocket;
        m_pUdpSocket = nullptr;
    }
    
    m_bConnected = false;
    UpdateConnectionState(false);
    LogMessage("UDP连接已断开");
}

void MainWindow::OnSendClicked()
{
    if (!m_bConnected || !m_pUdpSocket) {
        QMessageBox::warning(this, "警告", "请先连接UDP");
        return;
    }
    
    QString hexString = ui->lineEditSendData->text().trimmed();
    if (hexString.isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入要发送的数据");
        return;
    }
    
    // 转换十六进制字符串为字节数组
    QByteArray data = HexStringToByteArray(hexString);
    if (data.isEmpty()) {
        QMessageBox::warning(this, "警告", "无效的十六进制数据");
        return;
    }
    
    // 发送数据
    qint64 bytesWritten = m_pUdpSocket->writeDatagram(data, m_remoteAddress, m_remotePort);
    if (bytesWritten == -1) {
        LogMessage(QString("发送失败: %1").arg(m_pUdpSocket->errorString()), "ERROR");
    } else {
        LogMessage(QString("发送数据 -> %1:%2 [%3] (%4 bytes)")
                  .arg(m_remoteAddress.toString())
                  .arg(m_remotePort)
                  .arg(ByteArrayToHexString(data))
                  .arg(bytesWritten), "SEND");
    }
}

void MainWindow::OnClearSendClicked()
{
    ui->lineEditSendData->clear();
}

void MainWindow::OnClearLogClicked()
{
    ui->textEditLog->clear();
}

void MainWindow::OnReadyRead()
{
    while (m_pUdpSocket && m_pUdpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_pUdpSocket->pendingDatagramSize());
        
        QHostAddress sender;
        quint16 senderPort;
        
        qint64 bytesRead = m_pUdpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        
        if (bytesRead > 0) {
            datagram.resize(bytesRead);
            LogMessage(QString("接收数据 <- %1:%2 [%3] (%4 bytes)")
                      .arg(sender.toString())
                      .arg(senderPort)
                      .arg(ByteArrayToHexString(datagram))
                      .arg(bytesRead), "RECV");
            
            // 处理协议消息
            std::vector<uint8_t> data(datagram.begin(), datagram.end());
            ProcessProtocolMessage(data);
        }
    }
}

void MainWindow::OnSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    if (m_pUdpSocket) {
        LogMessage(QString("UDP错误: %1").arg(m_pUdpSocket->errorString()), "ERROR");
    }
}

void MainWindow::LogMessage(const QString &message, const QString &type)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString logEntry = QString("[%1] [%2] %3").arg(timestamp, type, message);
    
    // 显示在UI中
    ui->textEditLog->append(logEntry);
    
    // 自动滚动到底部
    ui->textEditLog->moveCursor(QTextCursor::End);
    
    // 写入文件（根据内存要求）
    WriteLogToFile(logEntry);
}

void MainWindow::WriteLogToFile(const QString &message)
{
    if (m_pLogStream) {
        *m_pLogStream << message << Qt::endl;
        m_pLogStream->flush();
    }
}

QByteArray MainWindow::HexStringToByteArray(const QString &hexString)
{
    QByteArray result;
    QString cleanHex = hexString.simplified().replace(" ", "").replace(",", "").toUpper();
    
    // 确保长度为偶数
    if (cleanHex.length() % 2 != 0) {
        return QByteArray();
    }
    
    bool ok;
    for (int i = 0; i < cleanHex.length(); i += 2) {
        QString hexByte = cleanHex.mid(i, 2);
        quint8 byte = hexByte.toUInt(&ok, 16);
        if (!ok) {
            return QByteArray();
        }
        result.append(byte);
    }
    
    return result;
}

QString MainWindow::ByteArrayToHexString(const QByteArray &data)
{
    QString result;
    for (int i = 0; i < data.size(); ++i) {
        result += QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
        if (i < data.size() - 1) {
            result += " ";
        }
    }
    return result;
}

void MainWindow::UpdateConnectionState(bool connected)
{
    ui->pushButtonConnect->setEnabled(!connected);
    ui->pushButtonDisconnect->setEnabled(connected);
    ui->pushButtonSend->setEnabled(connected);
    ui->pushButtonQueryDevices->setEnabled(connected);
    ui->pushButtonStart->setEnabled(connected);
    ui->pushButtonStop->setEnabled(connected && m_bDataViewRunning);
    
    // 更新状态栏
    if (connected) {
        statusBar()->showMessage("已连接");
    } else {
        statusBar()->showMessage("未连接");
    }
}

void MainWindow::OnQueryDevicesClicked()
{
    if (!m_bConnected || !m_pUdpSocket) {
        QMessageBox::warning(this, "警告", "请先连接UDP");
        return;
    }
    
    SendDeviceListRequest();
}

void MainWindow::SendDeviceListRequest()
{
    // 创建设备列表请求消息
    WhtsProtocol::Backend2Master::DeviceListReqMessage deviceListReq;
    deviceListReq.reserve = 0; // 保留字段
    
    // 使用协议处理器打包消息
    auto packets = m_pProtocolProcessor->packBackend2MasterMessage(deviceListReq);
    
    // 发送所有分片
    for (const auto& packet : packets) {
        QByteArray data(reinterpret_cast<const char*>(packet.data()), packet.size());
        qint64 bytesWritten = m_pUdpSocket->writeDatagram(data, m_remoteAddress, m_remotePort);
        
        if (bytesWritten == -1) {
            LogMessage(QString("发送设备列表请求失败: %1").arg(m_pUdpSocket->errorString()), "ERROR");
            return;
        } else {
            LogMessage(QString("发送设备列表请求 -> %1:%2 [%3] (%4 bytes)")
                      .arg(m_remoteAddress.toString())
                      .arg(m_remotePort)
                      .arg(ByteArrayToHexString(data))
                      .arg(bytesWritten), "SEND");
        }
    }
}

void MainWindow::ProcessProtocolMessage(const std::vector<uint8_t> &data)
{
    // 将数据传递给协议处理器
    m_pProtocolProcessor->processReceivedData(data);
    
    // 检查是否有完整的帧
    WhtsProtocol::Frame frame;
    while (m_pProtocolProcessor->getNextCompleteFrame(frame)) {
        // 解析Master2Backend消息
        std::unique_ptr<WhtsProtocol::Message> message;
        if (m_pProtocolProcessor->parseMaster2BackendPacket(frame.payload, message)) {
            // 检查消息类型
            if (message->getMessageId() == static_cast<uint8_t>(WhtsProtocol::Master2BackendMessageId::DEVICE_LIST_RSP_MSG)) {
                // 转换为设备列表响应消息
                auto deviceListResponse = dynamic_cast<WhtsProtocol::Master2Backend::DeviceListResponseMessage*>(message.get());
                if (deviceListResponse) {
                    HandleDeviceListResponse(*deviceListResponse);
                }
            }
            else if (message->getMessageId() == static_cast<uint8_t>(WhtsProtocol::Master2BackendMessageId::SLAVE_CFG_RSP_MSG)) {
                // 转换为从机配置响应消息
                auto slaveConfigResponse = dynamic_cast<WhtsProtocol::Master2Backend::SlaveConfigResponseMessage*>(message.get());
                if (slaveConfigResponse) {
                    HandleSlaveConfigResponse(*slaveConfigResponse);
                }
            }
        }
        
        // 解析Slave2Backend消息
        uint32_t slaveId;
        WhtsProtocol::DeviceStatus deviceStatus;
        std::unique_ptr<WhtsProtocol::Message> slave2BackendMessage;
        if (m_pProtocolProcessor->parseSlave2BackendPacket(frame.payload, slaveId, deviceStatus, slave2BackendMessage)) {
            // 检查消息类型
            if (slave2BackendMessage->getMessageId() == static_cast<uint8_t>(WhtsProtocol::Slave2BackendMessageId::CONDUCTION_DATA_MSG)) {
                // 转换为导通数据消息
                auto conductionDataMessage = dynamic_cast<WhtsProtocol::Slave2Backend::ConductionDataMessage*>(slave2BackendMessage.get());
                if (conductionDataMessage && m_bDataViewRunning) {
                    HandleConductionDataMessage(slaveId, deviceStatus, *conductionDataMessage);
                }
            }
        }
    }
}

void MainWindow::HandleDeviceListResponse(const WhtsProtocol::Master2Backend::DeviceListResponseMessage &message)
{
    LogMessage(QString("收到设备列表响应，设备数量: %1").arg(message.deviceCount), "INFO");
    
    // 更新设备表格
    UpdateDeviceTable(message.devices);
}

void MainWindow::UpdateDeviceTable(const std::vector<WhtsProtocol::Master2Backend::DeviceListResponseMessage::DeviceInfo> &devices)
{
    // 清空表格
    ui->tableWidgetDevices->setRowCount(0);
    
    // 添加设备信息
    for (size_t i = 0; i < devices.size(); ++i) {
        const auto& device = devices[i];
        
        ui->tableWidgetDevices->insertRow(i);
        
        // 设备ID (十六进制显示)
        ui->tableWidgetDevices->setItem(i, 0, 
            new QTableWidgetItem(QString("0x%1").arg(device.deviceId, 8, 16, QChar('0')).toUpper()));
        
        // 短ID
        ui->tableWidgetDevices->setItem(i, 1, 
            new QTableWidgetItem(QString::number(device.shortId)));
        
        // 在线状态
        QString onlineStatus = device.online ? "在线" : "离线";
        ui->tableWidgetDevices->setItem(i, 2, 
            new QTableWidgetItem(onlineStatus));
        
        // 版本信息
        QString version = QString("%1.%2.%3")
            .arg(device.versionMajor)
            .arg(device.versionMinor)
            .arg(device.versionPatch);
        ui->tableWidgetDevices->setItem(i, 3, 
            new QTableWidgetItem(version));
        
        // 电池电量（使用自定义控件）
        QWidget* batteryWidget = CreateBatteryWidget(device.batteryLevel);
        ui->tableWidgetDevices->setCellWidget(i, 4, batteryWidget);
    }
    
    // 调整列宽
    ui->tableWidgetDevices->resizeColumnsToContents();
}

QWidget* MainWindow::CreateBatteryWidget(uint8_t batteryLevel)
{
    // 创建容器widget
    QWidget* container = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(container);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(5);
    
    // 创建进度条
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(batteryLevel);
    progressBar->setFixedHeight(20);
    progressBar->setTextVisible(false); // 不显示进度条自带的文本
    
    // 根据电量设置进度条颜色
    QString styleSheet;
    if (batteryLevel >= 60) {
        // 电量充足 - 绿色
        styleSheet = "QProgressBar::chunk { background-color: #4CAF50; }";
    } else if (batteryLevel >= 30) {
        // 电量中等 - 黄色
        styleSheet = "QProgressBar::chunk { background-color: #FF9800; }";
    } else {
        // 电量不足 - 红色
        styleSheet = "QProgressBar::chunk { background-color: #F44336; }";
    }
    progressBar->setStyleSheet(styleSheet);
    
    // 创建显示百分比的标签
    QLabel* percentLabel = new QLabel(QString("%1%").arg(batteryLevel));
    percentLabel->setFixedWidth(35);
    percentLabel->setAlignment(Qt::AlignCenter);
    
    // 添加到布局
    layout->addWidget(progressBar, 1); // 进度条占主要空间
    layout->addWidget(percentLabel, 0); // 百分比标签固定宽度
    
    return container;
}

void MainWindow::OnAddSlaveConfigClicked()
{
    SlaveConfigDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        SlaveConfigData configData = dialog.getConfigData();
        m_slaveConfigs.append(configData);
        SaveSlaveConfigs();
        UpdateSlaveConfigTable();
        LogMessage(QString("添加从机配置: %1").arg(configData.name), "INFO");
    }
}

void MainWindow::OnEditSlaveConfigClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    int row = button->property("row").toInt();
    if (row >= 0 && row < m_slaveConfigs.size()) {
        SlaveConfigDialog dialog(m_slaveConfigs[row], this);
        if (dialog.exec() == QDialog::Accepted) {
            m_slaveConfigs[row] = dialog.getConfigData();
            SaveSlaveConfigs();
            UpdateSlaveConfigTable();
            LogMessage(QString("编辑从机配置: %1").arg(m_slaveConfigs[row].name), "INFO");
        }
    }
}

void MainWindow::OnDeleteSlaveConfigClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    int row = button->property("row").toInt();
    if (row >= 0 && row < m_slaveConfigs.size()) {
        QString configName = m_slaveConfigs[row].name;
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "确认删除", 
            QString("确定要删除配置 \"%1\" 吗？").arg(configName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            m_slaveConfigs.removeAt(row);
            SaveSlaveConfigs();
            UpdateSlaveConfigTable();
            LogMessage(QString("删除从机配置: %1").arg(configName), "INFO");
        }
    }
}

void MainWindow::OnSendSlaveConfigClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    int row = button->property("row").toInt();
    if (row >= 0 && row < m_slaveConfigs.size()) {
        if (!m_bConnected || !m_pUdpSocket) {
            QMessageBox::warning(this, "警告", "请先连接UDP");
            return;
        }
        
        SendSlaveConfig(m_slaveConfigs[row]);
    }
}

void MainWindow::LoadSlaveConfigs()
{
    m_slaveConfigs.clear();
    
    int size = m_pSettings->beginReadArray("SlaveConfigs");
    for (int i = 0; i < size; ++i) {
        m_pSettings->setArrayIndex(i);
        SlaveConfigData config;
        QString jsonString = m_pSettings->value("config").toString();
        if (config.fromJsonString(jsonString)) {
            m_slaveConfigs.append(config);
        }
    }
    m_pSettings->endArray();
    
    UpdateSlaveConfigTable();
}

void MainWindow::SaveSlaveConfigs()
{
    m_pSettings->beginWriteArray("SlaveConfigs");
    for (int i = 0; i < m_slaveConfigs.size(); ++i) {
        m_pSettings->setArrayIndex(i);
        m_pSettings->setValue("config", m_slaveConfigs[i].toJsonString());
    }
    m_pSettings->endArray();
    m_pSettings->sync();
}

void MainWindow::UpdateSlaveConfigTable()
{
    ui->tableWidgetSlaveConfigs->setRowCount(m_slaveConfigs.size());
    
    for (int i = 0; i < m_slaveConfigs.size(); ++i) {
        const SlaveConfigData& config = m_slaveConfigs[i];
        
        // 配置名称（只读）
        QTableWidgetItem* nameItem = new QTableWidgetItem(config.name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidgetSlaveConfigs->setItem(i, 0, nameItem);
        
        // 配置信息（只读）
        QTableWidgetItem* infoItem = new QTableWidgetItem(config.getConfigDescription());
        infoItem->setFlags(infoItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidgetSlaveConfigs->setItem(i, 1, infoItem);
        
        // 操作按钮
        QWidget* actionWidget = CreateSlaveConfigActionWidget(i);
        ui->tableWidgetSlaveConfigs->setCellWidget(i, 2, actionWidget);
    }
}

QWidget* MainWindow::CreateSlaveConfigActionWidget(int row)
{
    QWidget* container = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(container);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(3);
    
    QPushButton* sendButton = new QPushButton("发送", container);
    QPushButton* editButton = new QPushButton("编辑", container);
    QPushButton* copyButton = new QPushButton("复制", container);
    QPushButton* deleteButton = new QPushButton("删除", container);
    
    sendButton->setProperty("row", row);
    editButton->setProperty("row", row);
    copyButton->setProperty("row", row);
    deleteButton->setProperty("row", row);
    
    sendButton->setFixedSize(45, 25);
    editButton->setFixedSize(45, 25);
    copyButton->setFixedSize(45, 25);
    deleteButton->setFixedSize(45, 25);
    
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::OnSendSlaveConfigClicked);
    connect(editButton, &QPushButton::clicked, this, &MainWindow::OnEditSlaveConfigClicked);
    connect(copyButton, &QPushButton::clicked, this, &MainWindow::OnCopySlaveConfigClicked);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::OnDeleteSlaveConfigClicked);
    
    layout->addWidget(sendButton);
    layout->addWidget(editButton);
    layout->addWidget(copyButton);
    layout->addWidget(deleteButton);
    layout->addStretch();
    
    return container;
}

void MainWindow::SendSlaveConfig(const SlaveConfigData& configData)
{
    // 使用协议处理器打包消息
    auto packets = m_pProtocolProcessor->packBackend2MasterMessage(configData.config);
    
    // 发送所有分片
    for (const auto& packet : packets) {
        QByteArray data(reinterpret_cast<const char*>(packet.data()), packet.size());
        qint64 bytesWritten = m_pUdpSocket->writeDatagram(data, m_remoteAddress, m_remotePort);
        
        if (bytesWritten == -1) {
            LogMessage(QString("发送从机配置失败: %1").arg(m_pUdpSocket->errorString()), "ERROR");
            return;
        } else {
            LogMessage(QString("发送从机配置 \"%1\" -> %2:%3 [%4] (%5 bytes)")
                      .arg(configData.name)
                      .arg(m_remoteAddress.toString())
                      .arg(m_remotePort)
                      .arg(ByteArrayToHexString(data))
                      .arg(bytesWritten), "SEND");
        }
    }
}

void MainWindow::HandleSlaveConfigResponse(const WhtsProtocol::Master2Backend::SlaveConfigResponseMessage &message)
{
    QString statusText;
    if (message.status == 0) {
        statusText = "成功";
        LogMessage(QString("从机配置响应: 状态=%1, 从机数量=%2")
                  .arg(statusText).arg(message.slaveNum), "INFO");
    } else {
        statusText = "失败";
        LogMessage(QString("从机配置响应: 状态=%1, 从机数量=%2")
                  .arg(statusText).arg(message.slaveNum), "ERROR");
    }
    
    QMessageBox::information(this, "从机配置响应", 
                           QString("配置结果: %1\n从机数量: %2")
                           .arg(statusText).arg(message.slaveNum));
}

void MainWindow::OnCopySlaveConfigClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    int row = button->property("row").toInt();
    if (row >= 0 && row < m_slaveConfigs.size()) {
        // 复制配置数据
        SlaveConfigData copiedConfig = m_slaveConfigs[row];
        
        // 修改配置名称，避免重复
        copiedConfig.name = copiedConfig.name + "_副本";
        
        // 确保配置名称的唯一性
        int copyIndex = 1;
        QString baseName = copiedConfig.name;
        while (true) {
            bool nameExists = false;
            for (const auto& config : m_slaveConfigs) {
                if (config.name == copiedConfig.name) {
                    nameExists = true;
                    break;
                }
            }
            
            if (!nameExists) {
                break;
            }
            
            copyIndex++;
            copiedConfig.name = baseName + QString("(%1)").arg(copyIndex);
        }
        
        // 将复制的配置插入到当前行的下一行
        m_slaveConfigs.insert(row + 1, copiedConfig);
        
        // 保存配置并更新表格
        SaveSlaveConfigs();
        UpdateSlaveConfigTable();
        
        LogMessage(QString("复制从机配置: %1 -> %2").arg(m_slaveConfigs[row].name).arg(copiedConfig.name), "INFO");
    }
}

void MainWindow::OnStartClicked()
{
    if (!m_bConnected || !m_pUdpSocket) {
        QMessageBox::warning(this, "警告", "请先连接UDP");
        return;
    }
    
    // 发送启动控制消息
    SendCtrlMessage(1); // 1表示启动
    m_bDataViewRunning = true;
    
    // 更新按钮状态
    ui->pushButtonStart->setEnabled(false);
    ui->pushButtonStop->setEnabled(true);
    
    LogMessage("发送启动控制消息", "INFO");
}

void MainWindow::OnStopClicked()
{
    if (!m_bConnected || !m_pUdpSocket) {
        QMessageBox::warning(this, "警告", "请先连接UDP");
        return;
    }
    
    // 发送停止控制消息
    SendCtrlMessage(0); // 0表示停止
    m_bDataViewRunning = false;
    
    // 更新按钮状态
    ui->pushButtonStart->setEnabled(true);
    ui->pushButtonStop->setEnabled(false);
    
    LogMessage("发送停止控制消息", "INFO");
}

void MainWindow::SendCtrlMessage(uint8_t runningStatus)
{
    // 创建控制消息
    WhtsProtocol::Backend2Master::CtrlMessage ctrlMsg;
    ctrlMsg.runningStatus = runningStatus;
    
    // 使用协议处理器打包消息
    auto packets = m_pProtocolProcessor->packBackend2MasterMessage(ctrlMsg);
    
    // 发送所有分片
    for (const auto& packet : packets) {
        QByteArray data(reinterpret_cast<const char*>(packet.data()), packet.size());
        qint64 bytesWritten = m_pUdpSocket->writeDatagram(data, m_remoteAddress, m_remotePort);
        
        if (bytesWritten == -1) {
            LogMessage(QString("发送控制消息失败: %1").arg(m_pUdpSocket->errorString()), "ERROR");
            return;
        } else {
            LogMessage(QString("发送控制消息 (状态=%1) -> %2:%3 [%4] (%5 bytes)")
                      .arg(runningStatus)
                      .arg(m_remoteAddress.toString())
                      .arg(m_remotePort)
                      .arg(ByteArrayToHexString(data))
                      .arg(bytesWritten), "SEND");
        }
    }
}

void MainWindow::HandleConductionDataMessage(uint32_t slaveId, const WhtsProtocol::DeviceStatus& deviceStatus, const WhtsProtocol::Slave2Backend::ConductionDataMessage& message)
{
    LogMessage(QString("收到导通数据消息 - 从机ID: 0x%1, 数据长度: %2")
              .arg(slaveId, 8, 16, QChar('0')).toUpper()
              .arg(message.conductionLength), "INFO");
    
    // 更新数据查看表格
    UpdateDataViewTable(slaveId, deviceStatus, message);
}

void MainWindow::UpdateDataViewTable(uint32_t slaveId, const WhtsProtocol::DeviceStatus& deviceStatus, const WhtsProtocol::Slave2Backend::ConductionDataMessage& message)
{
    // 查找是否已存在该从机ID的行
    int targetRow = -1;
    for (int i = 0; i < ui->tableWidgetDataView->rowCount(); ++i) {
        QTableWidgetItem* item = ui->tableWidgetDataView->item(i, 0);
        if (item) {
            bool ok;
            uint32_t existingId = item->text().toUInt(&ok, 16);
            if (ok && existingId == slaveId) {
                targetRow = i;
                break;
            }
        }
    }
    
    // 如果没有找到，创建新行
    if (targetRow == -1) {
        targetRow = ui->tableWidgetDataView->rowCount();
        ui->tableWidgetDataView->insertRow(targetRow);
        
        // 设置从机ID
        ui->tableWidgetDataView->setItem(targetRow, 0, 
            new QTableWidgetItem(QString("0x%1").arg(slaveId, 8, 16, QChar('0')).toUpper()));
    }
    
    // 更新设备状态列（CS, SL, EUB, BLA, PS, EL1, EL2, A1, A2）
    ui->tableWidgetDataView->setItem(targetRow, 1, new QTableWidgetItem(deviceStatus.colorSensor ? "1" : "0"));
    ui->tableWidgetDataView->setItem(targetRow, 2, new QTableWidgetItem(deviceStatus.sleeveLimit ? "1" : "0"));
    ui->tableWidgetDataView->setItem(targetRow, 3, new QTableWidgetItem(deviceStatus.electromagnetUnlockButton ? "1" : "0"));
    ui->tableWidgetDataView->setItem(targetRow, 4, new QTableWidgetItem(deviceStatus.batteryLowAlarm ? "1" : "0"));
    ui->tableWidgetDataView->setItem(targetRow, 5, new QTableWidgetItem(deviceStatus.pressureSensor ? "1" : "0"));
    ui->tableWidgetDataView->setItem(targetRow, 6, new QTableWidgetItem(deviceStatus.electromagneticLock1 ? "1" : "0"));
    ui->tableWidgetDataView->setItem(targetRow, 7, new QTableWidgetItem(deviceStatus.electromagneticLock2 ? "1" : "0"));
    ui->tableWidgetDataView->setItem(targetRow, 8, new QTableWidgetItem(deviceStatus.accessory1 ? "1" : "0"));
    ui->tableWidgetDataView->setItem(targetRow, 9, new QTableWidgetItem(deviceStatus.accessory2 ? "1" : "0"));
    
    // 更新导通数据列
    QString conductionDataStr = ConductionDataToString(message.conductionData);
    ui->tableWidgetDataView->setItem(targetRow, 10, new QTableWidgetItem(conductionDataStr));
    
    // 自动滚动到最新数据
    ui->tableWidgetDataView->scrollToItem(ui->tableWidgetDataView->item(targetRow, 0));
}

QString MainWindow::DeviceStatusToString(const WhtsProtocol::DeviceStatus& status)
{
    QStringList statusList;
    if (status.colorSensor) statusList << "CS";
    if (status.sleeveLimit) statusList << "SL";
    if (status.electromagnetUnlockButton) statusList << "EUB";
    if (status.batteryLowAlarm) statusList << "BLA";
    if (status.pressureSensor) statusList << "PS";
    if (status.electromagneticLock1) statusList << "EL1";
    if (status.electromagneticLock2) statusList << "EL2";
    if (status.accessory1) statusList << "A1";
    if (status.accessory2) statusList << "A2";
    
    return statusList.join(",");
}

QString MainWindow::ConductionDataToString(const std::vector<uint8_t>& data)
{
    QString result;
    for (size_t i = 0; i < data.size(); ++i) {
        result += QString("%1").arg(data[i], 2, 16, QChar('0')).toUpper();
        if (i < data.size() - 1) {
            result += " ";
        }
    }
    return result;
}

void MainWindow::OnClearDataClicked()
{
    // 清除数据查看表格中的所有数据
    ui->tableWidgetDataView->setRowCount(0);
    
    LogMessage("清除数据查看表格", "INFO");
}
