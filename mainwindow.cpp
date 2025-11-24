#include "mainwindow.h"
#include "./ui_mainwindow.h"

#define LIGHT_SENSOR_DEVICE_PATH                "/dev/i2c-1"
#define LIGHT_SENSOR_DEVICE_ADDRESS             0x23

#define TEMPERATURE_HUMIDITY_DEVICE_PATH        "/dev/i2c-1"
#define TEMPERATURE_HUMIDITY_DEVICE_ADDRESS     0x44

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(&mReadSensorDataTimer, &QTimer::timeout, [&]() {
        TemperatureHumiditySensor::CelsiusHumidityValue celsiusHumidityValue;
        int luxValue;

        luxValue = mLightSensor.readLuxValue();
        celsiusHumidityValue = mTemperatureHumiditySensor.readCelsiusHumidityValue();

        ui->temperatureLabel->setText(QString("%1°C").arg((int)celsiusHumidityValue.temperature));
        ui->humidityLabel->setText(QString("%1%").arg((int)celsiusHumidityValue.humidity));
        ui->luxLabel->setText(QString("%1lx").arg(luxValue));

        if (m_client->state() == QMqttClient::Connected) {
            m_client->publish(QMqttTopicName("smartfarm/sensor/temp"), 
                              QString::number(celsiusHumidityValue.temperature, 'f', 1).toUtf8());
            m_client->publish(QMqttTopicName("smartfarm/sensor/humidity"), 
                              QString::number(celsiusHumidityValue.humidity, 'f', 1).toUtf8());
            m_client->publish(QMqttTopicName("smartfarm/sensor/lux"), 
                              QString::number(luxValue).toUtf8());
        }
    });

    m_client = new QMqttClient(this);
    m_client->setHostname("localhost");
    m_client->setPort(1883);

    connect(m_client, &QMqttClient::connected, this, &MainWindow::onMqttConnected);
    connect(m_client, &QMqttClient::messageReceived, this, &MainWindow::onMqttMessageReceived);

    m_client->connectToHost();
}

void MainWindow::onMqttConnected()
{
    printf("Connected to MQTT Broker!\n");
    auto subscription = m_client->subscribe(QMqttTopicFilter("smartfarm/control"), 0);
    if (!subscription) {
        printf("Could not subscribe to smartfarm/control\n");
    }
}

void MainWindow::onMqttMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    Q_UNUSED(topic);
    QString msg = QString::fromUtf8(message);

    if (msg == "LED1_ON") {
        mLightGpio.setState(GPIO_HIGH);
        ui->lightButton->setStyleSheet("background-color: green;");
        m_client->publish(QMqttTopicName("smartfarm/status"), "LED1_ON");
    } else if (msg == "LED1_OFF") {
        mLightGpio.setState(GPIO_LOW);
        ui->lightButton->setStyleSheet("background-color: gray;");
        m_client->publish(QMqttTopicName("smartfarm/status"), "LED1_OFF");
    } else if (msg == "LED2_ON") {
        mPumpGpio.setState(GPIO_HIGH);
        ui->pumpButton->setStyleSheet("background-color: green;");
        m_client->publish(QMqttTopicName("smartfarm/status"), "LED2_ON");
    } else if (msg == "LED2_OFF") {
        mPumpGpio.setState(GPIO_LOW);
        ui->pumpButton->setStyleSheet("background-color: gray;");
        m_client->publish(QMqttTopicName("smartfarm/status"), "LED2_OFF");  
    }

    updateGuiState();
}

void MainWindow::updateGuiState()
{
    if (mLightGpio.getState() == GPIO_HIGH)
        ui->lightButton->setStyleSheet("background-color: green;");
    else
        ui->lightButton->setStyleSheet("background-color: gray;");

    if (mPumpGpio.getState() == GPIO_HIGH)
        ui->pumpButton->setStyleSheet("background-color: green;");
    else
        ui->pumpButton->setStyleSheet("background-color: gray;");
}

MainWindow::~MainWindow()
{
    delete ui;
}

int MainWindow::init()
{
    int ret;

    ret = mLightSensor.init(LIGHT_SENSOR_DEVICE_PATH, LIGHT_SENSOR_DEVICE_ADDRESS);
    if (ret < 0)
    {
        printf("Failed to init light sensor\n");
        return -1;
    }

    ret = mTemperatureHumiditySensor.init(TEMPERATURE_HUMIDITY_DEVICE_PATH, TEMPERATURE_HUMIDITY_DEVICE_ADDRESS);
    if (ret < 0)
    {
        printf("Failed to init temperature humidity sensor\n");
        return -1;
    }

    ret = mLightGpio.init("light", "gpiochip2", 2); /* P8_7 pin */
    if (ret < 0)
    {
        printf("Failed to init light GPIO\n");
        return -1;
    } 
    else 
    {
        int state = mLightGpio.getState();
        if (state == GPIO_HIGH)
        {
            ui->lightButton->setStyleSheet("background-color: green;");
        }
        else if (state == GPIO_LOW)
        {
            ui->lightButton->setStyleSheet("background-color: gray;");
        }
    }

    ret = mPumpGpio.init("pump", "gpiochip2", 3); /* P8_8 pin */
    if (ret < 0)
    {
        printf("Failed to init pump GPIO\n");
        return -1;
    }
    else 
    {
        int state = mPumpGpio.getState();
        if (state == GPIO_HIGH)
        {
            ui->pumpButton->setStyleSheet("background-color: green;");
        }
        else if (state == GPIO_LOW)
        {
            ui->pumpButton->setStyleSheet("background-color: gray;");
        }
    }

    mReadSensorDataTimer.start(3000); // every 3000 ms
    return 0;
}

void MainWindow::on_lightButton_clicked()
{
    int state = mLightGpio.getState();
    int newState = (state == GPIO_HIGH) ? GPIO_LOW : GPIO_HIGH;
    mLightGpio.setState(newState);

    ui->lightButton->setStyleSheet(newState == GPIO_HIGH ? "background-color: green;" : "background-color: gray;");

    if (m_client->state() == QMqttClient::Connected) {
        m_client->publish(QMqttTopicName("smartfarm/status"), newState == GPIO_HIGH ? "LED1_ON" : "LED1_OFF");
    }
}

void MainWindow::on_pumpButton_clicked()
{
    int state = mPumpGpio.getState();
    int newState = (state == GPIO_HIGH) ? GPIO_LOW : GPIO_HIGH;
    mPumpGpio.setState(newState);
    ui->pumpButton->setStyleSheet(newState == GPIO_HIGH ? "background-color: green;" : "background-color: gray;");

    if (m_client->state() == QMqttClient::Connected) {
        m_client->publish(QMqttTopicName("smartfarm/status"), newState == GPIO_HIGH ? "LED2_ON" : "LED2_OFF");
    }
}
