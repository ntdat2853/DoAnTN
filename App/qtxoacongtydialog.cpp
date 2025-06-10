#include "qtxoacongtydialog.h"
#include "ui_qtxoacongtydialog.h"
#include "api.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QDebug>

QTXoaCongTyDialog::QTXoaCongTyDialog(const QString &token, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QTXoaCongTyDialog)
    , networkManager(new QNetworkAccessManager(this))
    , token(token)
{
    ui->setupUi(this);
    setWindowTitle("Xoá công ty");
    ui->tableWidgetXoaCongTy->setColumnCount(2);
    ui->tableWidgetXoaCongTy->setHorizontalHeaderLabels({"Công ty", "Chọn"});
    ui->tableWidgetXoaCongTy->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Kiểm tra kết nối tín hiệu
    bool connected = connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QTXoaCongTyDialog::on_pushButtonXacNhan_clicked);
    qDebug() << "pushButtonXacNhan connected:" << connected;
    connect(ui->pushButtonThoat, &QPushButton::clicked, this, &QTXoaCongTyDialog::on_pushButtonThoat_clicked);
}

QTXoaCongTyDialog::~QTXoaCongTyDialog()
{
    qDeleteAll(checkBoxes);
    delete ui;
}

void QTXoaCongTyDialog::loadCompanies(const QJsonArray &companies)
{
    qDebug() << "Loading companies, count:" << companies.size();
    ui->tableWidgetXoaCongTy->setRowCount(companies.size());
    checkBoxes.clear();

    for (int i = 0; i < companies.size(); ++i) {
        QJsonObject obj = companies[i].toObject();
        QString congTy = obj["CongTy"].toString();
        qDebug() << "Company loaded:" << congTy;

        QTableWidgetItem *congTyItem = new QTableWidgetItem(congTy);
        congTyItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidgetXoaCongTy->setItem(i, 0, congTyItem);

        QCheckBox *checkBox = new QCheckBox();
        checkBox->setStyleSheet("QCheckBox { margin-left: 50%; margin-right: 50%; }");
        checkBoxes.append(checkBox);
        ui->tableWidgetXoaCongTy->setCellWidget(i, 1, checkBox);
    }
}

void QTXoaCongTyDialog::on_pushButtonXacNhan_clicked()
{
    qDebug() << "pushButtonXacNhan clicked";
    QJsonArray congTyList;
    for (int i = 0; i < checkBoxes.size(); ++i) {
        if (checkBoxes[i]->isChecked()) {
            QString congTy = ui->tableWidgetXoaCongTy->item(i, 0)->text();
            congTyList.append(congTy);
        }
    }

    qDebug() << "CongTyList:" << congTyList;

    if (congTyList.isEmpty()) {
        QMessageBox::warning(this, "Cảnh báo", "Vui lòng chọn ít nhất một công ty để xóa.");
        qDebug() << "No companies selected";
        return;
    }

    if (token.isEmpty()) {
        QMessageBox::critical(this, "Lỗi", "Token không hợp lệ. Vui lòng đăng nhập lại.");
        qDebug() << "Empty token";
        return;
    }

    QJsonObject json;
    json["cong_ty_list"] = congTyList;

    QNetworkRequest request(QUrl(API+"/quanly/delete-by-congty/"));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    qDebug() << "Sending DELETE request with token:" << token.left(10) << "...";
    QNetworkReply *reply = networkManager->sendCustomRequest(request, "DELETE", QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleDeleteCongTyReply(reply);
        reply->deleteLater();
    });

    QMessageBox::information(this, "Đang xử lý", "Yêu cầu xóa công ty đang được gửi. Vui lòng chờ.");
    accept();

    disconnect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QTXoaCongTyDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QTXoaCongTyDialog::on_pushButtonXacNhan_clicked);
}

void QTXoaCongTyDialog::on_pushButtonThoat_clicked()
{
    qDebug() << "pushButtonThoat clicked";
    reject();
}

void QTXoaCongTyDialog::handleDeleteCongTyReply(QNetworkReply *reply)
{
    qDebug() << "Handling delete company reply";
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = "Không thể xóa công ty: " + reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("detail")) {
                errorMsg = "Lỗi từ server: " + obj["detail"].toString();
            }
        }
        qDebug() << errorMsg;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();
    if (obj.contains("detail") && obj["detail"].toString() == "Deleted companies successfully") {
        qDebug() << "Successfully deleted companies";
        emit companiesDeleted();
    } else {
        qDebug() << "Failed to delete companies:" << obj["detail"].toString();
    }
}
