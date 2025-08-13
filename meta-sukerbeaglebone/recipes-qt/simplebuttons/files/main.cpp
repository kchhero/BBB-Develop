#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Simple Buttons");

    QVBoxLayout *layout = new QVBoxLayout(&window);

    QPushButton *button1 = new QPushButton("Button 1");
    QPushButton *button2 = new QPushButton("Button 2");

    // 초기 색상 설정
    button1->setStyleSheet("background-color: red");
    button2->setStyleSheet("background-color: blue");

    // 버튼 클릭 시 색상 토글 람다 함수
    auto toggleColor = [](QPushButton *btn) {
        static bool toggle = false;
        if (toggle)
            btn->setStyleSheet("background-color: red");
        else
            btn->setStyleSheet("background-color: blue");
        toggle = !toggle;
    };

    QObject::connect(button1, &QPushButton::clicked, [=]() {
        toggleColor(button1);
    });

    QObject::connect(button2, &QPushButton::clicked, [=]() {
        toggleColor(button2);
    });

    layout->addWidget(button1);
    layout->addWidget(button2);

    window.setLayout(layout);
    window.resize(200, 100);
    window.show();

    return app.exec();
}
