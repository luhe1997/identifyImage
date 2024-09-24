#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include<QCamera>
#include<QCameraViewfinder>
#include<QCamera>
#include<QCameraImageCapture>
#include<QNetworkAccessManager>
#include<QSslConfiguration>
#include<QNetworkReply>
#include<QJsonParseError>
#include<QJsonDocument>
#include<QJsonObject>
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT
public slots:
    void showCamera(int id,QImage preview);
    void takePicture();
    void tokenReply(QNetworkReply *reply);
    void beginFaceDetect();
    void imgReply(QNetworkReply *reply);
public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;
    QCamera *camera;
    QCameraViewfinder *finder;
    QCameraImageCapture *imageCapture;
    QTimer *refreshTimer;
    QNetworkAccessManager *tokenManager;
    QNetworkAccessManager *imgManager;
    QSslConfiguration sslConfig;
    QString accessToken;
    QImage img;
};
#endif // WIDGET_H
