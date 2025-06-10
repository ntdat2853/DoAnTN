#include "qlxoakhachhangdialog.h"
#include "ui_qlxoakhachhangdialog.h"
#include "api.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QDebug>
#include <QTableWidgetItem>
#include <QCheckBox>
#include <QHeaderView>

QLXoaKhachHangDialog::QLXoaKhachHangDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QLXoaKhachHangDialog)
    , networkManager(new QNetworkAccessManager(this))
    , deletionIndex(0)
{
    ui->setupUi(this);
    setWindowTitle("Xoá khách hàng");

    // Thiết lập bảng với 2 cột: Tên khách hàng và Chọn
    ui->tableWidgetXoaKhachHang->setColumnCount(2);
    ui->tableWidgetXoaKhachHang->setHorizontalHeaderLabels({"Tên khách hàng", "Chọn"});
    ui->tableWidgetXoaKhachHang->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    QFont headerFont = ui->tableWidgetXoaKhachHang->horizontalHeader()->font();
    headerFont.setBold(true);
    ui->tableWidgetXoaKhachHang->horizontalHeader()->setFont(headerFont);

    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QLXoaKhachHangDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonThoat, &QPushButton::clicked, this, &QLXoaKhachHangDialog::on_pushButtonThoat_clicked);
}

QLXoaKhachHangDialog::~QLXoaKhachHangDialog()
{
    delete ui;
}

void QLXoaKhachHangDialog::setTokenAndTenDangNhap(const QString &token, const QString &tenDangNhap, const QString &congTy)
{
    this->token = token;
    this->tenDangNhap = tenDangNhap;
    this->congTy = congTy;
    qDebug() << "Loading customers for CongTy:" << congTy << "with TenDangNhap:" << tenDangNhap;
    loadKhachHangList();
}

void QLXoaKhachHangDialog::loadKhachHangList()
{
    if (congTy.isEmpty()) {
        qDebug() << "CongTy is empty, cannot load customers";
        QMessageBox::critical(this, "Lỗi", "Không thể tải danh sách khách hàng: Tên công ty trống");
        return;
    }

    QNetworkRequest request(QUrl(API+"/khachhang/?cong_ty=" + QUrl::toPercentEncoding(congTy)));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    qDebug() << "Sending request to:" << request.url().toString();
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleKhachHangReply(reply);
        reply->deleteLater();
    });
}

void QLXoaKhachHangDialog::handleKhachHangReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = "Không thể lấy danh sách khách hàng: " + reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("detail")) {
                errorMsg += "\nChi tiết: " + obj["detail"].toString();
            }
        }
        qDebug() << "API error:" << errorMsg;
        QMessageBox::critical(this, "Lỗi", errorMsg);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonArray array = doc.array();
    qDebug() << "Received" << array.size() << "customers from API";

    ui->tableWidgetXoaKhachHang->setRowCount(0); // Xóa bảng trước khi thêm dữ liệu mới
    int row = 0;
    for (int i = 0; i < array.size(); ++i) {
        QJsonObject obj = array[i].toObject();

        // Kiểm tra tính hợp lệ của IDKhachHang, HoVaTen, ten_dang_nhap, và SoDienThoai
        if (!obj.contains("IDKhachHang") || !obj.contains("HoVaTen") || !obj.contains("ten_dang_nhap") || !obj.contains("SoDienThoai")) {
            qDebug() << "Bỏ qua bản ghi thiếu dữ liệu: " << obj;
            continue;
        }

        int khachhang_id = obj["IDKhachHang"].toInt();
        if (khachhang_id <= 0) {
            qDebug() << "Bỏ qua bản ghi với IDKhachHang không hợp lệ: IDKhachHang=" << khachhang_id;
            continue;
        }

        // Thêm hàng mới vào bảng
        ui->tableWidgetXoaKhachHang->insertRow(row);

        // Cột Tên khách hàng
        QString displayText = obj["HoVaTen"].toString() + " (" + obj["ten_dang_nhap"].toString() + ")";
        QTableWidgetItem *nameItem = new QTableWidgetItem(displayText);
        nameItem->setTextAlignment(Qt::AlignCenter);
        nameItem->setData(Qt::UserRole, khachhang_id);
        ui->tableWidgetXoaKhachHang->setItem(row, 0, nameItem);

        // Cột Chọn (Checkbox)
        QWidget *checkBoxWidget = new QWidget();
        QCheckBox *checkBox = new QCheckBox();
        QHBoxLayout *layout = new QHBoxLayout(checkBoxWidget);
        layout->addWidget(checkBox);
        layout->setAlignment(Qt::AlignCenter);
        layout->setContentsMargins(0, 0, 0, 0);
        ui->tableWidgetXoaKhachHang->setCellWidget(row, 1, checkBoxWidget);

        row++;
    }

    if (row == 0) {
        qDebug() << "No valid customers found for CongTy:" << congTy;
        QMessageBox::information(this, "Thông báo", "Không có khách hàng nào thuộc công ty này để xóa");
    }
}

void QLXoaKhachHangDialog::on_pushButtonXacNhan_clicked()
{
    pendingDeletions.clear();
    deletionIndex = 0;

    // Thu thập danh sách khách hàng được chọn
    for (int i = 0; i < ui->tableWidgetXoaKhachHang->rowCount(); ++i) {
        QWidget *checkBoxWidget = ui->tableWidgetXoaKhachHang->cellWidget(i, 1);
        QCheckBox *checkBox = checkBoxWidget ? checkBoxWidget->findChild<QCheckBox*>() : nullptr;
        if (checkBox && checkBox->isChecked()) {
            QTableWidgetItem *nameItem = ui->tableWidgetXoaKhachHang->item(i, 0);
            if (!nameItem) {
                qDebug() << "Bỏ qua hàng " << i << " do thiếu nameItem";
                continue;
            }
            int khachhang_id = nameItem->data(Qt::UserRole).toInt();
            if (khachhang_id <= 0) {
                qDebug() << "Bỏ qua hàng " << i << " do IDKhachHang không hợp lệ: IDKhachHang=" << khachhang_id;
                continue;
            }
            pendingDeletions.append(qMakePair(khachhang_id, khachhang_id)); // Lưu IDKhachHang
        }
    }

    if (pendingDeletions.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Vui lòng chọn ít nhất một khách hàng để xóa");
        return;
    }

    // Hiển thị hộp thoại xác nhận
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Xác nhận xóa",
        QString("Bạn có chắc chắn muốn xóa %1 khách hàng? Hành động này sẽ xóa tất cả thông tin liên quan (tài khoản, giao dịch, thanh toán).").arg(pendingDeletions.size()),
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::No) {
        return;
    }

    // Bắt đầu xóa khách hàng đầu tiên
    if (!pendingDeletions.isEmpty()) {
        int khachhang_id = pendingDeletions[0].first;
        QNetworkRequest request(QUrl(API+"/khachhang/" + QString::number(khachhang_id)));
        request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
        qDebug() << "Sending DELETE request for IDKhachHang:" << khachhang_id;
        QNetworkReply *deleteReply = networkManager->deleteResource(request);
        connect(deleteReply, &QNetworkReply::finished, this, [=]() {
            handleDeleteReply(deleteReply);
            deleteReply->deleteLater();
        });
    }

    disconnect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QLXoaKhachHangDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QLXoaKhachHangDialog::on_pushButtonXacNhan_clicked);
}

void QLXoaKhachHangDialog::handleDeleteReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = "Không thể xóa khách hàng: " + reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("detail")) {
                errorMsg += "\nChi tiết: " + obj["detail"].toString();
            }
        }
        qDebug() << "Delete error:" << errorMsg;
        QMessageBox::critical(this, "Lỗi", errorMsg);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (!obj.contains("detail") || !obj["detail"].toString().contains("Đã xóa khách hàng")) {
        QString errorMsg = "Xóa khách hàng thất bại: " + obj["detail"].toString();
        qDebug() << "Delete failed:" << errorMsg;
        QMessageBox::critical(this, "Lỗi", errorMsg);
        return;
    }

    // Xóa thành công, chuyển sang khách hàng tiếp theo
    deletionIndex++;
    if (deletionIndex < pendingDeletions.size()) {
        int khachhang_id = pendingDeletions[deletionIndex].first;
        QNetworkRequest request(QUrl(API+"/khachhang/" + QString::number(khachhang_id)));
        request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
        qDebug() << "Sending DELETE request for IDKhachHang:" << khachhang_id;
        QNetworkReply *deleteReply = networkManager->deleteResource(request);
        connect(deleteReply, &QNetworkReply::finished, this, [=]() {
            handleDeleteReply(deleteReply);
            deleteReply->deleteLater();
        });
    } else {
        // Hoàn tất xóa
        qDebug() << "Completed deletion of" << pendingDeletions.size() << "customers";
        QMessageBox::information(this, "Thành công", QString("Đã xóa thành công %1 khách hàng").arg(pendingDeletions.size()));
        emit customerDeleted();
        accept();
    }
}

void QLXoaKhachHangDialog::on_pushButtonThoat_clicked()
{
    reject();
}
