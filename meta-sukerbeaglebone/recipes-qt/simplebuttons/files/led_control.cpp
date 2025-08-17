#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFile>
#include <QDebug>

// 드라이버에 신호를 보내는 함수
void toggleLed(int ledIndex) {
    QFile devFile("/dev/my_leds");
    if (!devFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open /dev/my_leds";
        return;
    }
    char data = '0' + ledIndex;
    devFile.write(&data, 1);
    devFile.close();
    qDebug() << "Toggled LED" << ledIndex;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QWidget window;
    window.setWindowTitle("LED Control");

    QHBoxLayout *layout = new QHBoxLayout(&window);

    for (int i = 0; i < 4; ++i) {
        QPushButton *button = new QPushButton(QString("LED %1").arg(i + 1));
        layout->addWidget(button);
        // 버튼 클릭 시, 해당 람다 함수가 호출되도록 연결
        QObject::connect(button, &QPushButton::clicked, [i]() {
            toggleLed(i);
        });
    }

    window.setLayout(layout);
    window.show();
    return app.exec();
}