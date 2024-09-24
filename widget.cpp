#include "widget.h"
#include "ui_widget.h"
#include<QCamera>
#include<QCameraViewfinder>
#include<QHBoxLayout>//水平盒子
#include<QVBoxLayout>
#include<QTimer>
#include<QNetworkAccessManager>
#include<QUrl>
#include<QUrlQuery>
#include<QSslSocket>
#include<QBuffer>
#include<QJsonArray>
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    //设置摄像头功能
    camera = new QCamera();//摄像头
    finder = new QCameraViewfinder();//取景器
    imageCapture = new QCameraImageCapture(camera);
    camera->setViewfinder(finder);//咔嚓
    camera->setCaptureMode(QCamera::CaptureStillImage);//
    imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToFile);//每次拍的文件

    connect(imageCapture,&QCameraImageCapture::imageCaptured,this,&Widget::showCamera);

    connect(ui->pushButton,&QPushButton::clicked,this,&Widget::beginFaceDetect);
    camera->start();

    //界面布局
    this->resize(1200,700);
    QVBoxLayout *vboxl= new QVBoxLayout();
    vboxl->addWidget(ui->label);
    vboxl->addWidget(ui->pushButton);

    QVBoxLayout *vboxr= new QVBoxLayout();
    vboxr->addWidget(finder);
    vboxr->addWidget(ui->textBrowser);

    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->addLayout(vboxl);
    hbox->addLayout(vboxr);
    //hbox->addWidget(finder);
    this->setLayout(hbox);

    //利用定时器不断刷新界面，不断拍照
    refreshTimer = new QTimer(this);
    connect(refreshTimer,&QTimer::timeout,this,&Widget::takePicture);
    refreshTimer->start(20);

    //访问网络
    tokenManager = new QNetworkAccessManager(this);
    //一旦接收到reply，就相应槽函数
    connect(tokenManager,&QNetworkAccessManager::finished,this,&Widget::tokenReply);
    qDebug()<<tokenManager->supportedSchemes();

    imgManager = new QNetworkAccessManager(this);
    connect(imgManager,&QNetworkAccessManager::finished,this,&Widget::imgReply);

    QUrl url("https://aip.baidubce.com/oauth/2.0/token");
    qDebug()<<url;

    QUrlQuery query;//在url拼接键值对
    query.addQueryItem("grant_type","client_credentials");
    query.addQueryItem("client_id","FnkVQbfIBKmiysvFf5aCHxVM");
    query.addQueryItem("client_secret","NOirtArxc09qArpzsRZTS59AKbcWRpEg");

    url.setQuery(query);
    qDebug()<<url;

    if(QSslSocket::supportsSsl()){
        qDebug()<<"支持";
    }else{
        qDebug()<<"不支持";
    }
    //配置ssl
    sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::QueryPeer);
    sslConfig.setProtocol(QSsl::TlsV1_2);
    //组装请求
    QNetworkRequest req;
    req.setUrl(url);
    req.setSslConfiguration(sslConfig);
    //发送get请求
    tokenManager->get(req);


}
void  Widget::showCamera(int id,QImage img){
    Q_UNUSED(id);
    this->img = img;
    ui->label->setPixmap(QPixmap::fromImage(img));
}
void Widget::takePicture(){
    imageCapture->capture();
}
void Widget::tokenReply(QNetworkReply *reply){
    //错误处理
    if(reply->error()!=QNetworkReply::NoError){
        qDebug()<<reply->errorString();
        return;
    }
    //正常应答
    const QByteArray reply_data = reply->readAll();
    //qDebug()<<reply_data;
    //json解析
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(reply_data,&parseError);

    //解析成功
    if(parseError.error==QJsonParseError::NoError){
        QJsonObject obj =  doc.object();
        if(obj.contains("access_token")){
            accessToken=  obj.take("access_token").toString();
        }
        ui->textBrowser->setText(accessToken);
    }else{
        qDebug()<<"json error:"<<parseError.errorString();
    }
}

void Widget::beginFaceDetect(){
    //拼接base64编码
    QByteArray ba;
    QBuffer buffer(&ba);
    img.save(&buffer,"png");
    QString b64str = ba.toBase64();
    //qDebug()<<b64str;
    //请求体Body参数设置
    QJsonObject postJson;
    QJsonDocument doc;
    postJson.insert("image",b64str);
    postJson.insert("image_type","BASE64");
    postJson.insert("face_field","age,expression,face_shape,gender,glasses,emotion,face_type,mask,beauty");

    doc.setObject(postJson);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);

    //组装图像识别请求
    QUrl url("https://aip.baidubce.com/rest/2.0/face/v3/detect");
    QUrlQuery query;
    query.addQueryItem("access_token",accessToken);
    url.setQuery(query);

    //组装请求
    QNetworkRequest req;
    req.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/json"));
    req.setUrl(url);
    req.setSslConfiguration(sslConfig);
    imgManager->post(req,postData);
}

void Widget::imgReply(QNetworkReply *reply){
    //错误处理
    if(reply->error()!=QNetworkReply::NoError){
        qDebug()<<reply->errorString();
        return;
    }
    //正常应答
    const QByteArray reply_data = reply->readAll();
    //qDebug()<<reply_data;

    QJsonParseError jsonErr;
    QJsonDocument doc = QJsonDocument::fromJson(reply_data,&jsonErr);

    if(jsonErr.error==QJsonParseError::NoError){

        QString faceInfo;

        //取出最外层的json
        QJsonObject obj = doc.object();
        if(obj.contains("result")){
            QJsonObject resultObj= obj.take("result").toObject();
            //取出人脸列表
            if(resultObj.contains("face_list")){
                QJsonArray faceList = resultObj.take("face_list").toArray();
                //取出第一张人脸信息
                QJsonObject faceObject = faceList.at(0).toObject();
                //取出年龄
                if(faceObject.contains("age")){
                    double age = faceObject.take("age").toDouble();
                    faceInfo.append("年龄:").append(QString::number(age)).append("\r\n");

                }
                //取出性别
                if(faceObject.contains("gender")){
                    QJsonObject genderObj = faceObject.take("gender").toObject();
                    if(genderObj.contains("type")){
                        QString gender = genderObj.take("type").toString();
                        faceInfo.append("性别:").append(gender).append("\r\n");
                    }
                }
                //取出情绪
                if(faceObject.contains("emotion")){
                    QJsonObject emotionObj = faceObject.take("emotion").toObject();
                    if(emotionObj.contains("type")){
                        QString emotion = emotionObj.take("type").toString();
                        faceInfo.append("表情:").append(emotion).append("\r\n");
                    }
                }
                //取出是否戴口罩
                if(faceObject.contains("mask")){
                    QJsonObject maskObj = faceObject.take("mask").toObject();
                    if(maskObj.contains("type")){
                        int mask = maskObj.take("type").toInt();
                        faceInfo.append("是否戴口罩:").append(mask==0?"没戴口罩":"戴口罩").append("\r\n");
                    }
                }
                //取出颜值
                if(faceObject.contains("beauty")){
                    double beauty = faceObject.take("beauty").toDouble();
                    faceInfo.append("颜值:").append(QString::number(beauty)).append("\r\n");

                }
                //脸型
                if(faceObject.contains("face_shape")){
                    QJsonObject faceObj = faceObject.take("face_shape").toObject();
                    if(faceObj.contains("type")){
                        QString  faceType = faceObj.take("type").toString();
                        faceInfo.append("脸型:").append(faceType).append("\r\n");
                    }
                }
            }
        }
        ui->textBrowser->setText(faceInfo);
    }else{
        qDebug()<<"json error:"<<jsonErr.errorString();
    }
}
Widget::~Widget()
{
    delete ui;
}

