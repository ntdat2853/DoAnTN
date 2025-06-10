from sqlalchemy import Column, Integer, String, Date, Enum, Float, ForeignKey, DateTime, DECIMAL, Time, ForeignKeyConstraint
from sqlalchemy.orm import relationship
from database import Base
import enum
import datetime  # Import đầy đủ module datetime

# Define VaiTro enum for user roles
class VaiTro(enum.Enum):
    KhachHang = "KhachHang"
    QuanLy = "QuanLy"
    QuanTri = "QuanTri"

# TaiKhoan model
class TaiKhoan(Base):
    __tablename__ = "TaiKhoan"

    IDTaiKhoan = Column(Integer, primary_key=True, autoincrement=True)
    TenDangNhap = Column(String(50), nullable=False, unique=True)
    MatKhau = Column(String(255), nullable=False)
    NgayTao = Column(Date, nullable=False)
    VaiTro = Column(Enum(VaiTro, native_enum=False, values_callable=lambda obj: [e.value for e in obj]), nullable=False)

    quanly = relationship("QuanLy", back_populates="taikhoan", uselist=False, lazy='noload')
    khachhang = relationship("KhachHang", back_populates="taikhoan", uselist=False, lazy='noload')
    quantri = relationship("QuanTri", back_populates="taikhoan", uselist=False, lazy='noload')

# QuanLy model
class QuanLy(Base):
    __tablename__ = "QuanLy"

    IDQuanLy = Column(Integer, primary_key=True, autoincrement=True)
    IDTaiKhoan = Column(Integer, ForeignKey("TaiKhoan.IDTaiKhoan"), nullable=False, unique=True)
    HoVaTen = Column(String(100))
    SoDienThoai = Column(String(15))
    Gmail = Column(String(100))
    CongTy = Column(String(100), unique=True)

    taikhoan = relationship("TaiKhoan", back_populates="quanly")
    khachhangs = relationship("KhachHang", back_populates="quanly")

# KhachHang model
class KhachHang(Base):
    __tablename__ = "KhachHang"

    IDKhachHang = Column(Integer, primary_key=True, autoincrement=True)
    IDTaiKhoan = Column(Integer, ForeignKey("TaiKhoan.IDTaiKhoan"), nullable=False, unique=True)
    HoVaTen = Column(String(100))
    Gmail = Column(String(100))
    RFID = Column(String(50))
    SoDienThoai = Column(String(15))
    CongTy = Column(String(100), ForeignKey("QuanLy.CongTy"))
    SoTaiKhoan = Column(String(50))
    NganHang = Column(String(100))

    taikhoan = relationship("TaiKhoan", back_populates="khachhang")
    quanly = relationship("QuanLy", back_populates="khachhangs")
    giaodichs = relationship("GiaoDich", back_populates="khachhang")
    thanhtoans = relationship("ThanhToan", back_populates="khachhang")

# QuanTri model
class QuanTri(Base):
    __tablename__ = "QuanTri"

    IDQuanTri = Column(Integer, primary_key=True, autoincrement=True)
    IDTaiKhoan = Column(Integer, ForeignKey("TaiKhoan.IDTaiKhoan"), nullable=False, unique=True)
    HoVaTen = Column(String(100))
    SoDienThoai = Column(String(15))
    Gmail = Column(String(100))

    taikhoan = relationship("TaiKhoan", back_populates="quantri")

# GiaMu model
class GiaMu(Base):
    __tablename__ = "GiaMu"

    IDGiaMu = Column(Integer, primary_key=True, autoincrement=True)
    NgayThietLap = Column(Date, nullable=False)
    CongTy = Column(String(100), ForeignKey("QuanLy.CongTy"), nullable=False)
    GiaMuNuoc = Column(DECIMAL(10, 2), nullable=False)
    GiaMuTap = Column(DECIMAL(10, 2), nullable=False)



# GiaoDich model
class GiaoDich(Base):
    __tablename__ = "GiaoDich"

    IDGiaoDich = Column(Integer, primary_key=True, autoincrement=True)
    IDKhachHang = Column(Integer, ForeignKey("KhachHang.IDKhachHang"), nullable=False)
    NgayGiaoDich = Column(Date, nullable=False)
    CongTy = Column(String(100), nullable=False)
    ThoiGianGiaoDich = Column(Time, nullable=False)
    MuNuoc = Column(DECIMAL(10, 2))
    TSC = Column(DECIMAL(10, 2))
    GiaMuNuoc = Column(DECIMAL(10, 2))
    MuTap = Column(DECIMAL(10, 2))
    DRC = Column(DECIMAL(10, 2))
    GiaMuTap = Column(DECIMAL(10, 2))
    TongTien = Column(DECIMAL(10, 2))  # Generated column in the database

    khachhang = relationship("KhachHang", back_populates="giaodichs")

    __table_args__ = (
        ForeignKeyConstraint(
            ['NgayGiaoDich', 'CongTy'],
            ['GiaMu.NgayThietLap', 'GiaMu.CongTy'],
            name='fk_giaodich_giamu'
        ),
        ForeignKeyConstraint(
            ['CongTy'],
            ['QuanLy.CongTy'],
            name='fk_giaodich_quanly'
        ),
    )

# ThanhToan model
class ThanhToan(Base):
    __tablename__ = "ThanhToan"

    IDThanhToan = Column(Integer, primary_key=True, autoincrement=True)
    IDKhachHang = Column(Integer, ForeignKey("KhachHang.IDKhachHang"), nullable=False)
    Thang = Column(String(7), nullable=False)  # Format: YYYY-MM
    TongMuNuoc = Column(DECIMAL(10, 2))
    TongMuTap = Column(DECIMAL(10, 2))
    TongThanhToan = Column(DECIMAL(10, 2))

    khachhang = relationship("KhachHang", back_populates="thanhtoans")

    __table_args__ = (
        {'mysql_unique': ['IDKhachHang', 'Thang']},  # Ràng buộc UNIQUE trên (IDKhachHang, Thang)
    )

# TriggerLog model
class TriggerLog(Base):
    __tablename__ = "TriggerLog"

    LogID = Column(Integer, primary_key=True, autoincrement=True)
    Message = Column(String(255))
    LogTime = Column(DateTime, default=datetime.datetime.utcnow)  # Sử dụng datetime.datetime.utcnow