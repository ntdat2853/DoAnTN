#include "dangnhapwindow.h"
#include "ui_dangnhapwindow.h"
#include "khachhangwindow.h"
#include "quanlywindow.h"
#include "quantriwindow.h"
#include "api.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDebug>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QString>
#include <QTime>

DangNhapWindow::DangNhapWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DangNhapWindow)
    , networkManager(new QNetworkAccessManager(this))
    , khachHangWindow(nullptr)
    , quanLyWindow(nullptr)
    , quanTriWindow(nullptr)
    , sendGridApiKey("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")
    , isLoginProcessed(false)
{
    ui->setupUi(this);
    setWindowTitle("Đăng nhập");
    setAttribute(Qt::WA_QuitOnClose, false); // Ngăn ứng dụng thoát khi đóng cửa sổ
    connect(ui->pushButtonDangNhap, &QPushButton::clicked, this, &DangNhapWindow::on_pushButtonDangNhap_clicked);
    connect(ui->pushButtonQuenMatKhau, &QPushButton::clicked, this, &DangNhapWindow::on_pushButtonQuenMatKhau_clicked);
}

DangNhapWindow::~DangNhapWindow()
{
    delete ui;
    delete khachHangWindow;
    delete quanLyWindow;
    delete quanTriWindow;
}

void DangNhapWindow::on_pushButtonDangNhap_clicked()
{
    if (isLoginProcessed) {
        return; // Ngăn chặn xử lý nếu đã đăng nhập
    }

    QString tenDangNhap = ui->lineEditTenDangNhap->text();
    QString matKhau = ui->lineEditMatKhau->text();

    if (tenDangNhap.isEmpty() || matKhau.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Vui lòng nhập tên đăng nhập và mật khẩu");
        return;
    }

    QJsonObject json;
    json["TenDangNhap"] = tenDangNhap;
    json["MatKhau"] = matKhau;

    QNetworkRequest request(QUrl(API+"/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleLoginReply(reply);
        reply->deleteLater();
    });

    ui->pushButtonDangNhap->setEnabled(false); // Vô hiệu hóa nút để ngăn nhấn lại
}

void DangNhapWindow::on_pushButtonQuenMatKhau_clicked()
{
    QString tenDangNhap = ui->lineEditTenDangNhap->text();

    if (tenDangNhap.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Vui lòng nhập tên đăng nhập");
        return;
    }

    // Kiểm tra xem tên đăng nhập có tồn tại không
    QNetworkRequest request(QUrl(API+"/taikhoan/by-ten-dang-nhap/" + tenDangNhap));
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleCheckTenDangNhapReply(reply);
        reply->deleteLater();
    });

    disconnect(ui->pushButtonQuenMatKhau, &QPushButton::clicked, this, &DangNhapWindow::on_pushButtonQuenMatKhau_clicked);
    connect(ui->pushButtonQuenMatKhau, &QPushButton::clicked, this, &DangNhapWindow::on_pushButtonQuenMatKhau_clicked);
}

void DangNhapWindow::handleCheckTenDangNhapReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể kiểm tra tên đăng nhập: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("detail") && obj["detail"].toString() == "TaiKhoan not found") {
        QMessageBox::warning(this, "Lỗi", "Tên đăng nhập không tồn tại. Vui lòng nhập lại.");
        return;
    }

    // Lưu tên đăng nhập và vai trò
    tenDangNhap = obj["TenDangNhap"].toString();
    vaiTro = obj["VaiTro"].toString();

    // Hiển thị dialog nhập Gmail
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Xác nhận Gmail");
    QVBoxLayout *layout = new QVBoxLayout(dialog);

    QLabel *label = new QLabel("Nhập Gmail của bạn:", dialog);
    QLineEdit *gmailInput = new QLineEdit(dialog);
    QPushButton *submitButton = new QPushButton("Xác nhận", dialog);

    layout->addWidget(label);
    layout->addWidget(gmailInput);
    layout->addWidget(submitButton);
    dialog->setLayout(layout);

    connect(submitButton, &QPushButton::clicked, this, [=]() {
        enteredGmail = gmailInput->text();
        if (enteredGmail.isEmpty()) {
            QMessageBox::warning(dialog, "Lỗi", "Vui lòng nhập Gmail");
            return;
        }

        // Lấy thông tin người dùng để kiểm tra Gmail
        QString endpoint;
        if (vaiTro == "KhachHang") {
            endpoint = "khachhang-info";
        } else if (vaiTro == "QuanLy") {
            endpoint = "quanly-info";
        } else if (vaiTro == "QuanTri") {
            endpoint = "quantri-info";
        } else {
            QMessageBox::critical(dialog, "Lỗi", "Vai trò không hợp lệ");
            dialog->reject();
            return;
        }

        QNetworkRequest request(QUrl(API+"/" + endpoint + "/" + tenDangNhap));
        QNetworkReply *infoReply = networkManager->get(request);
        connect(infoReply, &QNetworkReply::finished, this, [=]() {
            handleGetUserInfoReply(infoReply);
            infoReply->deleteLater();
        });

        dialog->accept();
    });

    dialog->exec();
    delete dialog;
}

void DangNhapWindow::handleGetUserInfoReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể lấy thông tin người dùng: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("detail") && obj["detail"].toString().contains("not found")) {
        QMessageBox::warning(this, "Lỗi", "Không tìm thấy thông tin người dùng.");
        return;
    }

    QString userGmail = obj["Gmail"].toString();
    if (userGmail.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Tài khoản không có Gmail. Vui lòng liên hệ quản trị viên.");
        return;
    }

    if (enteredGmail != userGmail) {
        QMessageBox::warning(this, "Lỗi", "Gmail không khớp với tài khoản. Vui lòng thử lại.");
        return;
    }

    // Tạo mật khẩu ngẫu nhiên
    newPassword = generateRandomPassword();

    // Gửi email thực sự qua SendGrid
    sendEmail(userGmail, newPassword);

    // Cập nhật mật khẩu mới qua API
    QJsonObject json;
    json["new_password"] = newPassword;

    QNetworkRequest request(QUrl(API+"/taikhoan/reset-password/" + tenDangNhap));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *updateReply = networkManager->put(request, QJsonDocument(json).toJson());
    connect(updateReply, &QNetworkReply::finished, this, [=]() {
        handleUpdatePasswordReply(updateReply);
        updateReply->deleteLater();
    });
}

void DangNhapWindow::sendEmail(const QString &toEmail, const QString &newPassword)
{
    QJsonObject emailData;

    QJsonObject toObject;
    toObject["email"] = toEmail;

    QJsonArray toArray;
    toArray.append(toObject);

    QJsonObject personalization;
    personalization["to"] = toArray;

    QJsonArray personalizationsArray;
    personalizationsArray.append(personalization);
    emailData["personalizations"] = personalizationsArray;

    QJsonObject fromObject;
    fromObject["email"] = "ntdat2853@gmail.com";
    emailData["from"] = fromObject;

    emailData["subject"] = "Mật Khẩu Mới - Quản Lý Mủ Cao Su";

    QJsonObject contentObject;
    contentObject["type"] = "text/plain";
    contentObject["value"] = "Mật khẩu mới của bạn là: " + newPassword + "\nVui lòng đăng nhập và đổi mật khẩu ngay sau khi nhận được email này.";

    QJsonArray contentArray;
    contentArray.append(contentObject);
    emailData["content"] = contentArray;

    QNetworkRequest request(QUrl("https://api.sendgrid.com/v3/mail/send"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + sendGridApiKey).toUtf8());

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(emailData).toJson());
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleSendEmailReply(reply);
        reply->deleteLater();
    });
}

void DangNhapWindow::handleSendEmailReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể gửi email: " + reply->errorString());
        QMessageBox::information(this, "Thông báo", "Mật khẩu mới của bạn là: " + newPassword);
        return;
    }

    QMessageBox::information(this, "Thành công", "Mật khẩu mới đã được gửi đến Gmail của bạn. Vui lòng kiểm tra hộp thư.");
}

void DangNhapWindow::handleUpdatePasswordReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể cập nhật mật khẩu: " + reply->errorString());
        QMessageBox::information(this, "Thông báo", "Mật khẩu mới của bạn là: " + newPassword);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("detail") && obj["detail"].toString() == "Mật khẩu đã được đặt lại") {
        QMessageBox::information(this, "Thành công", "Mật khẩu đã được cập nhật. Vui lòng đăng nhập lại.");
        ui->lineEditMatKhau->clear();
    } else {
        QMessageBox::critical(this, "Lỗi", "Cập nhật mật khẩu thất bại: " + obj["detail"].toString());
        QMessageBox::information(this, "Thông báo", "Mật khẩu mới của bạn là: " + newPassword);
    }
}

QString DangNhapWindow::generateRandomPassword()
{
    const QString upperCase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const QString lowerCase = "abcdefghijklmnopqrstuvwxyz";
    const QString numbers = "0123456789";
    const QString specialChars = "!@#$%^&*()_+-=[]{}|;:,.<>?";

    QString password;
    QRandomGenerator generator(QTime::currentTime().msec());

    password += upperCase[generator.bounded(upperCase.length())];
    password += lowerCase[generator.bounded(lowerCase.length())];
    password += numbers[generator.bounded(numbers.length())];
    password += specialChars[generator.bounded(specialChars.length())];

    QString allChars = upperCase + lowerCase + numbers + specialChars;
    for (int i = 0; i < 4; ++i) {
        password += allChars[generator.bounded(allChars.length())];
    }

    for (int i = 0; i < password.length(); ++i) {
        int j = generator.bounded(password.length());
        QChar temp = password[i];
        password[i] = password[j];
        password[j] = temp;
    }

    return password;
}

void DangNhapWindow::handleLoginReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Đăng nhập thất bại: " + reply->errorString());
        ui->pushButtonDangNhap->setEnabled(true); // Bật lại nút đăng nhập
        isLoginProcessed = false;
        return;
    }

    if (isLoginProcessed) {
        return; // Ngăn xử lý nếu đã tạo cửa sổ
    }

    isLoginProcessed = true; // Đánh dấu đã xử lý đăng nhập

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("access_token") && obj.contains("token_type")) {
        token = obj["access_token"].toString();

        QStringList tokenParts = token.split('.');
        if (tokenParts.size() == 3) {
            QByteArray payload = QByteArray::fromBase64(tokenParts[1].toUtf8());
            QJsonDocument payloadDoc = QJsonDocument::fromJson(payload);
            QJsonObject payloadObj = payloadDoc.object();
            QString vaiTro = payloadObj["vai_tro"].toString();
            QString tenDangNhap = payloadObj["sub"].toString();

            if (vaiTro == "KhachHang") {
                khachHangWindow = new KhachHangWindow(token, tenDangNhap, nullptr);
                khachHangWindow->show();
                this->close();
            } else if (vaiTro == "QuanLy") {
                quanLyWindow = new QuanLyWindow(token, tenDangNhap, nullptr);
                quanLyWindow->show();
                this->close();
            } else if (vaiTro == "QuanTri") {
                quanTriWindow = new QuanTriWindow(token, tenDangNhap, nullptr);
                quanTriWindow->show();
                this->close();
            } else {
                QMessageBox::critical(this, "Lỗi", "Vai trò không hợp lệ");
                ui->pushButtonDangNhap->setEnabled(true);
                isLoginProcessed = false;
            }
        } else {
            QMessageBox::critical(this, "Lỗi", "Token không hợp lệ");
            ui->pushButtonDangNhap->setEnabled(true);
            isLoginProcessed = false;
        }
    } else {
        QMessageBox::critical(this, "Lỗi", "Đăng nhập thất bại: " + obj["detail"].toString());
        ui->pushButtonDangNhap->setEnabled(true);
        isLoginProcessed = false;
    }
}
