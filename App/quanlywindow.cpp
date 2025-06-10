#include "quanlywindow.h"
#include "ui_quanlywindow.h"
#include "dangnhapwindow.h"
#include "doimatkhaudialog.h"
#include "qlthongtindialog.h" // Thêm include
#include "qlthemkhachhangdialog.h" // Thêm include
#include "qlxoakhachhangdialog.h" // Thêm include
#include "qlgiamudialog.h" // Thêm include
#include "api.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDebug>
#include <QFont>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

QuanLyWindow::QuanLyWindow(const QString &token, const QString &tenDangNhap, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::QuanLyWindow)
    , networkManager(new QNetworkAccessManager(this))
    , token(token)
    , tenDangNhap(tenDangNhap)
    , dangNhapWindow(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("Quản lý");
    setAttribute(Qt::WA_QuitOnClose, false);

    // Kết nối các nút
    connect(ui->pushButtonDangXuat, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonDangXuat_clicked);
    connect(ui->pushButtonThemKhachHang, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonThemKhachHang_clicked);
    connect(ui->pushButtonXoaKhachHang, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonXoaKhachHang_clicked);
    connect(ui->pushButtonNhapGia, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonNhapGia_clicked);
    connect(ui->pushButtonDoiMatKhau, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonDoiMatKhau_clicked);
    connect(ui->pushButtonThongTinQuanLy, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonThongTinQuanLy_clicked);

    // Lấy thông tin quản lý để lấy CongTy
    QNetworkRequest quanLyRequest(QUrl(API+"/quanly-info/" + tenDangNhap));
    quanLyRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *quanLyReply = networkManager->get(quanLyRequest);
    connect(quanLyReply, &QNetworkReply::finished, this, [=]() {
        handleQuanLyInfoReply(quanLyReply);
        quanLyReply->deleteLater();
    });
}

QuanLyWindow::~QuanLyWindow()
{
    delete ui;
    delete dangNhapWindow;
}

void QuanLyWindow::on_pushButtonDangXuat_clicked()
{
    isLogoutProcessed = true;
    if (!isLogoutProcessed) {
        qDebug() << "Logout already processed, ignoring";
        return; // Ngăn xử lý nếu đã đăng xuất
    }

    qDebug() << "Processing logout for QuanTriWindow";
    isLogoutProcessed = false;

    // Vô hiệu hóa nút và ngắt kết nối tín hiệu
    ui->pushButtonDangXuat->setEnabled(isLogoutProcessed);
    disconnect(ui->pushButtonDangXuat, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonDangXuat_clicked);

    dangNhapWindow = new DangNhapWindow(nullptr);
    dangNhapWindow->show();
    this->close();
}

void QuanLyWindow::on_pushButtonThemKhachHang_clicked()
{
    if (findChild<QLThemKhachHangDialog*>()) {
        qDebug() << "QLThemKhachHangDialog already open, ignoring";
        return;
    }

    QLThemKhachHangDialog *dialog = new QLThemKhachHangDialog(this);
    dialog->setTokenAndTenDangNhap(token, tenDangNhap);
    connect(dialog, &QLThemKhachHangDialog::khachHangAdded, this, &QuanLyWindow::refreshInterface); // Kết nối signal
    dialog->exec();
    qDebug() << "QLThemKhachHangDialog closed";
    delete dialog;
    disconnect(ui->pushButtonThemKhachHang, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonThemKhachHang_clicked);
    connect(ui->pushButtonThemKhachHang, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonThemKhachHang_clicked);

    // Đóng QuanLyWindow hiện tại và mở instance mới
    qDebug() << "Closing current QuanLyWindow and opening new instance";
    QuanLyWindow *newWindow = new QuanLyWindow(token, tenDangNhap, nullptr);
    newWindow->show();
    this->close();
}

void QuanLyWindow::on_pushButtonXoaKhachHang_clicked()
{
    if (findChild<QLXoaKhachHangDialog*>()) {
        qDebug() << "QLXoaKhachHangDialog already open, ignoring";
        return;
    }

    QLXoaKhachHangDialog *dialog = new QLXoaKhachHangDialog(this);
    dialog->setTokenAndTenDangNhap(token, tenDangNhap, congTy);
    connect(dialog, &QLXoaKhachHangDialog::customerDeleted, this, &QuanLyWindow::refreshInterface);
    dialog->exec();
    qDebug() << "QLXoaKhachHangDialog closed";
    delete dialog;

    disconnect(ui->pushButtonXoaKhachHang, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonXoaKhachHang_clicked);
    connect(ui->pushButtonXoaKhachHang, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonXoaKhachHang_clicked);
}

void QuanLyWindow::on_pushButtonNhapGia_clicked()
{
    if (findChild<QLGiaMuDialog*>()) {
        qDebug() << "QLGiaMuDialog already open, ignoring";
        return;
    }

    QLGiaMuDialog *dialog = new QLGiaMuDialog(this);
    dialog->setTokenAndTenDangNhap(token, tenDangNhap);
    connect(dialog, &QLGiaMuDialog::giaMuUpdated, this, &QuanLyWindow::refreshInterface);
    dialog->exec();
    qDebug() << "QLGiaMuDialog closed";
    delete dialog;

    disconnect(ui->pushButtonNhapGia, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonNhapGia_clicked);
    connect(ui->pushButtonNhapGia, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonNhapGia_clicked);
}

void QuanLyWindow::on_pushButtonDoiMatKhau_clicked()
{
    DoiMatKhauDialog *dialog = new DoiMatKhauDialog(this);
    dialog->setTokenAndTenDangNhap(token, tenDangNhap);
    dialog->exec();
    delete dialog;
    disconnect(ui->pushButtonDoiMatKhau, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonDoiMatKhau_clicked);
    connect(ui->pushButtonDoiMatKhau, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonDoiMatKhau_clicked);
}

void QuanLyWindow::on_pushButtonThongTinQuanLy_clicked()
{
    if (findChild<QLThongTinDialog*>()) {
        qDebug() << "QLThongTinDialog already open, ignoring";
        return;
    }

    QLThongTinDialog *dialog = new QLThongTinDialog(this);
    dialog->setTokenAndTenDangNhap(token, tenDangNhap);
    connect(dialog, &QLThongTinDialog::infoUpdated, this, &QuanLyWindow::updateManagerName);
    dialog->exec();
    qDebug() << "QLThongTinDialog closed";
    delete dialog;

    disconnect(ui->pushButtonThongTinQuanLy, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonThongTinQuanLy_clicked);
    connect(ui->pushButtonThongTinQuanLy, &QPushButton::clicked, this, &QuanLyWindow::on_pushButtonThongTinQuanLy_clicked);
}

void QuanLyWindow::updateManagerName(const QString &hoVaTen)
{
    qDebug() << "Updating manager name to:" << hoVaTen;
    ui->pushButtonThongTinQuanLy->setText(hoVaTen.isEmpty() ? "Thông tin quản lý" : hoVaTen);
}

// Slot để làm mới giao diện (giả định QuanLyWindow có hàm refreshInterface)
void QuanLyWindow::refreshInterface()
{
    qDebug() << "Refreshing QuanLyWindow interface";
    if (congTy.isEmpty()) {
        qDebug() << "CongTy not set, cannot refresh";
        return;
    }

    // Làm mới bảng khách hàng
    QNetworkRequest khachHangRequest(QUrl(API+"/khachhang/?cong_ty=" + QUrl::toPercentEncoding(congTy)));
    khachHangRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *khachHangReply = networkManager->get(khachHangRequest);
    connect(khachHangReply, &QNetworkReply::finished, this, [=]() {
        handleKhachHangReply(khachHangReply);
        khachHangReply->deleteLater();
    });

    // Làm mới giá mủ
    QNetworkRequest giaMuRequest(QUrl(API+"/giamu/latest/" + QUrl::toPercentEncoding(congTy)));
    giaMuRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *giaMuReply = networkManager->get(giaMuRequest);
    connect(giaMuReply, &QNetworkReply::finished, this, [=]() {
        handleGiaMuReply(giaMuReply);
        giaMuReply->deleteLater();
    });
}

void QuanLyWindow::handleQuanLyInfoReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể lấy thông tin quản lý: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    congTy = obj["CongTy"].toString();
    QString hoVaTen = obj["HoVaTen"].toString();
    ui->pushButtonThongTinQuanLy->setText(hoVaTen.isEmpty() ? "Thông tin quản lý" : hoVaTen);

    // Lấy danh sách khách hàng theo CongTy
    QNetworkRequest khachHangRequest(QUrl(API+"/khachhang/?cong_ty=" + QUrl::toPercentEncoding(congTy)));
    khachHangRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *khachHangReply = networkManager->get(khachHangRequest);
    connect(khachHangReply, &QNetworkReply::finished, this, [=]() {
        handleKhachHangReply(khachHangReply);
        khachHangReply->deleteLater();
    });

    // Lấy giá mủ mới nhất theo CongTy
    QNetworkRequest giaMuRequest(QUrl(API+"/giamu/latest/" + QUrl::toPercentEncoding(congTy)));
    giaMuRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *giaMuReply = networkManager->get(giaMuRequest);
    connect(giaMuReply, &QNetworkReply::finished, this, [=]() {
        handleGiaMuReply(giaMuReply);
        giaMuReply->deleteLater();
    });
}

void QuanLyWindow::handleGiaMuReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = "Không thể lấy giá mủ: " + reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("detail")) {
                errorMsg += "\nChi tiết: " + obj["detail"].toString();
            }
        }
        QMessageBox::critical(this, "Lỗi", errorMsg);
        ui->labelGiaMuNuoc->setText("N/A");
        ui->labelGiaMuTap->setText("N/A");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.isEmpty()) {
        QMessageBox::critical(this, "Lỗi", "Không tìm thấy giá mủ cho công ty này");
        ui->labelGiaMuNuoc->setText("N/A");
        ui->labelGiaMuTap->setText("N/A");
        return;
    }

    QString giaMuNuoc = QString::number(obj["GiaMuNuoc"].toDouble(), 'f', 2);
    QString giaMuTap = QString::number(obj["GiaMuTap"].toDouble(), 'f', 2);

    ui->labelGiaMuNuoc->setText(giaMuNuoc);
    ui->labelGiaMuTap->setText(giaMuTap);
}

void QuanLyWindow::handleKhachHangReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể lấy danh sách khách hàng: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonArray array = doc.array();

    // Thiết lập bảng khách hàng (7 cột: Họ và tên, Số điện thoại, Gmail, RFID, Số tài khoản, Ngân hàng, Chi tiết)
    ui->tableWidgetThongTinKhachHang->setColumnCount(7);
    ui->tableWidgetThongTinKhachHang->setHorizontalHeaderLabels({
        "Họ và tên",
        "Số điện thoại",
        "Gmail",
        "RFID",
        "Số tài khoản",
        "Ngân hàng",
        "Chi tiết"
    });

    // In đậm tiêu đề các cột
    QFont headerFont = ui->tableWidgetThongTinKhachHang->horizontalHeader()->font();
    headerFont.setBold(true);
    ui->tableWidgetThongTinKhachHang->horizontalHeader()->setFont(headerFont);

    ui->tableWidgetThongTinKhachHang->setRowCount(array.size());
    ui->tableWidgetThongTinKhachHang->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    for (int i = 0; i < array.size(); ++i) {
        QJsonObject obj = array[i].toObject();

        // Tạo các item và căn giữa
        QTableWidgetItem *hoVaTenItem = new QTableWidgetItem(obj["HoVaTen"].toString());
        hoVaTenItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidgetThongTinKhachHang->setItem(i, 0, hoVaTenItem);

        QTableWidgetItem *soDienThoaiItem = new QTableWidgetItem(obj["SoDienThoai"].toString());
        soDienThoaiItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidgetThongTinKhachHang->setItem(i, 1, soDienThoaiItem);

        QTableWidgetItem *gmailItem = new QTableWidgetItem(obj["Gmail"].toString());
        gmailItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidgetThongTinKhachHang->setItem(i, 2, gmailItem);

        QTableWidgetItem *rfidItem = new QTableWidgetItem(obj["RFID"].toString());
        rfidItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidgetThongTinKhachHang->setItem(i, 3, rfidItem);

        QTableWidgetItem *soTaiKhoanItem = new QTableWidgetItem(obj["SoTaiKhoan"].toString());
        soTaiKhoanItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidgetThongTinKhachHang->setItem(i, 4, soTaiKhoanItem);

        QTableWidgetItem *nganHangItem = new QTableWidgetItem(obj["NganHang"].toString());
        nganHangItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidgetThongTinKhachHang->setItem(i, 5, nganHangItem);

        // Tạo nút Chi tiết với chữ nghiêng và gạch chân
        QPushButton *detailButton = new QPushButton("Chi tiết");
        QFont font = detailButton->font();
        font.setItalic(true);
        font.setUnderline(true);
        detailButton->setFont(font);
        detailButton->setStyleSheet("border: none;");
        ui->tableWidgetThongTinKhachHang->setCellWidget(i, 6, detailButton);

        // Lưu IDKhachHang và ten_dang_nhap vào userData của hoVaTenItem
        hoVaTenItem->setData(Qt::UserRole, obj["IDKhachHang"].toInt());
        hoVaTenItem->setData(Qt::UserRole + 1, obj["ten_dang_nhap"].toString());

        // Kết nối tín hiệu clicked của nút với slot, truyền số hàng
        connect(detailButton, &QPushButton::clicked, this, [=]() {
            onDetailButtonClicked(i);
        });
    }
}

void QuanLyWindow::onDetailButtonClicked(int row)
{
    // Lấy ten_dang_nhap từ userData của ô cột 0
    QTableWidgetItem *item = ui->tableWidgetThongTinKhachHang->item(row, 0);
    if (!item) {
        QMessageBox::critical(this, "Lỗi", "Không thể lấy thông tin khách hàng: Dữ liệu không hợp lệ");
        return;
    }
    QString tenDangNhap = item->data(Qt::UserRole + 1).toString();

    // Gửi yêu cầu lấy thông tin tài khoản từ endpoint /taikhoan/by-ten-dang-nhap/
    QNetworkRequest taiKhoanDetailRequest(QUrl(API+"/taikhoan/by-ten-dang-nhap/" + tenDangNhap));
    taiKhoanDetailRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *taiKhoanDetailReply = networkManager->get(taiKhoanDetailRequest);
    connect(taiKhoanDetailReply, &QNetworkReply::finished, this, [=]() {
        if (taiKhoanDetailReply->error() != QNetworkReply::NoError) {
            QString errorMsg = "Không thể lấy thông tin chi tiết tài khoản: " + taiKhoanDetailReply->errorString();
            QJsonDocument docDetail = QJsonDocument::fromJson(taiKhoanDetailReply->readAll());
            if (!docDetail.isNull() && docDetail.isObject()) {
                QJsonObject objDetail = docDetail.object();
                if (objDetail.contains("detail")) {
                    errorMsg += "\nChi tiết: " + objDetail["detail"].toString();
                }
            }
            QMessageBox::critical(this, "Lỗi", errorMsg);
            taiKhoanDetailReply->deleteLater();
            return;
        }

        QJsonDocument docDetail = QJsonDocument::fromJson(taiKhoanDetailReply->readAll());
        QJsonObject objDetail = docDetail.object();

        if (objDetail.isEmpty() || objDetail["VaiTro"].toString() != "KhachHang") {
            QMessageBox::critical(this, "Lỗi", "Không tìm thấy thông tin tài khoản khách hàng hoặc vai trò không hợp lệ");
            taiKhoanDetailReply->deleteLater();
            return;
        }

        showKhachHangDetails(objDetail);
        taiKhoanDetailReply->deleteLater();
    });
}

void QuanLyWindow::showKhachHangDetails(const QJsonObject &taikhoan)
{
    // Tạo dialog hiển thị thông tin
    QDialog *detailDialog = new QDialog(this);
    detailDialog->setWindowTitle("Thông tin tài khoản khách hàng");

    // Kích thước dialog giống giao diện quản trị (300x200)
    detailDialog->setFixedSize(300, 200);

    QVBoxLayout *layout = new QVBoxLayout(detailDialog);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *tenDangNhapLabel = new QLabel("Tên đăng nhập: " + taikhoan["TenDangNhap"].toString());
    tenDangNhapLabel->setAlignment(Qt::AlignCenter);

    QLabel *matKhauLabel = new QLabel("Mật khẩu: " + taikhoan["MatKhau"].toString());
    matKhauLabel->setAlignment(Qt::AlignCenter);

    QLabel *ngayTaoLabel = new QLabel("Ngày tạo: " + taikhoan["NgayTao"].toString());
    ngayTaoLabel->setAlignment(Qt::AlignCenter);

    QPushButton *closeButton = new QPushButton("Đóng");
    closeButton->setFixedWidth(100);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch();

    connect(closeButton, &QPushButton::clicked, detailDialog, &QDialog::close);

    layout->addWidget(tenDangNhapLabel);
    layout->addWidget(matKhauLabel);
    layout->addWidget(ngayTaoLabel);
    layout->addLayout(buttonLayout);
    detailDialog->setLayout(layout);

    detailDialog->exec();
}
