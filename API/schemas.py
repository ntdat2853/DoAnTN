from pydantic import BaseModel, EmailStr, validator
from datetime import date, time
from enum import Enum
from typing import Optional, List
from datetime import date, datetime

class VaiTro(str, Enum):
    KhachHang = "KhachHang"
    QuanLy = "QuanLy"
    QuanTri = "QuanTri"

class TaiKhoanBase(BaseModel):
    TenDangNhap: str
    MatKhau: str
    NgayTao: date
    VaiTro: VaiTro

class TaiKhoanCreate(TaiKhoanBase):
    pass

class TaiKhoan(TaiKhoanBase):
    IDTaiKhoan: int

    class Config:
        from_attributes = True  # Thay orm_mode thành from_attributes

class QuanLyBase(BaseModel):
    ten_dang_nhap: str
    HoVaTen: Optional[str] = None
    SoDienThoai: Optional[str] = None
    Gmail: Optional[EmailStr] = None
    CongTy: Optional[str] = None

class QuanLyCreate(BaseModel):
    TenDangNhap: str
    MatKhau: str
    HoVaTen: Optional[str] = None
    SODienThoai: Optional[str] = None
    Gmail: Optional[EmailStr] = None
    CongTy: str

class QuanLyUpdate(QuanLyBase):
    pass

class QuanLy(QuanLyBase):
    IDQuanLy: int
    IDTaiKhoan: int

    class Config:
        from_attributes = True  # Thay orm_mode thành from_attributes

class KhachHangBase(BaseModel):
    HoVaTen: Optional[str] = None
    Gmail: Optional[EmailStr] = None
    RFID: Optional[str] = None
    SoDienThoai: Optional[str] = None
    CongTy: Optional[str] = None
    SoTaiKhoan: Optional[str] = None
    NganHang: Optional[str] = None

class KhachHangCreate(KhachHangBase):
    TenDangNhap: str
    MatKhau: str
    NgayTao: date

class KhachHangUpdate(KhachHangBase):
    ten_dang_nhap: str

class KhachHang(KhachHangBase):
    IDKhachHang: int
    ten_dang_nhap: str  # Thêm trường ten_dang_nhap

    class Config:
        from_attributes = True

class QuanTriBase(BaseModel):
    ten_dang_nhap: str
    HoVaTen: Optional[str] = None
    SoDienThoai: Optional[str] = None
    Gmail: Optional[EmailStr] = None

class QuanTriCreate(BaseModel):
    TenDangNhap: str
    MatKhau: str
    HoVaTen: Optional[str] = None
    SoDienThoai: Optional[str] = None
    Gmail: Optional[EmailStr] = None

class QuanTriUpdate(QuanTriBase):
    pass

class QuanTri(QuanTriBase):
    IDQuanTri: int
    IDTaiKhoan: int

    class Config:
        from_attributes = True  # Thay orm_mode thành from_attributes

class GiaoDichBase(BaseModel):
    IDKhachHang: int
    NgayGiaoDich: date
    CongTy: str
    ThoiGianGiaoDich: time

class GiaoDichCreate(GiaoDichBase):
    KhoiLuongMuNuoc: float
    DoMuNuoc: float
    KhoiLuongMuTap: float
    DoMuTap: float

class GiaoDich(GiaoDichBase):
    IDGiaoDich: int
    KhoiLuongMuNuoc: float
    DoMuNuoc: float
    KhoiLuongMuTap: float
    DoMuTap: float
    MuNuoc: float
    TSC: float
    GiaMuNuoc: float
    MuTap: float
    DRC: float
    GiaMuTap: float
    TongTien: float

    class Config:
        from_attributes = True  # Thay orm_mode thành from_attributes

class ThanhToanBase(BaseModel):
    IDKhachHang: int
    Thang: str
    TongMuNuoc: float
    TongMuTap: float
    TongThanhToan: float

class ThanhToan(ThanhToanBase):
    IDThanhToan: int

    class Config:
        from_attributes = True  # Thay orm_mode thành from_attributes

class TriggerLog(BaseModel):
    LogID: int
    Message: Optional[str] = None
    LogTime: datetime

    class Config:
        from_attributes = True  # Thay orm_mode thành from_attributes

class LoginRequest(BaseModel):
    TenDangNhap: str
    MatKhau: str

class Token(BaseModel):
    access_token: str
    token_type: str

class GiaMuBase(BaseModel):
    NgayThietLap: str
    CongTy: str
    GiaMuNuoc: float
    GiaMuTap: float

    @validator('GiaMuNuoc', 'GiaMuTap')
    def check_gia_mu(cls, v):
        if v <= 0:
            raise ValueError('GiaMu must be greater than 0')
        return v

class GiaMuCreate(GiaMuBase):
    pass

class GiaMuUpdate(GiaMuBase):
    pass

class GiaMu(GiaMuBase):
    IDGiaMu: int

    class Config:
        from_attributes = True  # Thay orm_mode thành from_attributes
        json_encoders = {
            date: lambda v: v.strftime("%Y-%m-%d")  # Chuyển datetime.date thành chuỗi
        }

class PasswordUpdateRequest(BaseModel):
    current_password: str
    new_password: str

class PasswordResetRequest(BaseModel):
    new_password: str

# Add these at the end of schemas.py
class GiaoDichMuTapCreate(BaseModel):
    RFID: str
    KhoiLuongMuTap: float

    @validator('KhoiLuongMuTap')
    def check_mu_tap(cls, v):
        if v <= 0:
            raise ValueError('KhoiLuongMuTap must be greater than 0')
        return v

    @validator('RFID')
    def check_rfid(cls, v):
        if not v:
            raise ValueError('RFID cannot be empty')
        return v

class GiaoDichMuNuocCreate(BaseModel):
    RFID: str
    KhoiLuongMuNuoc: float

    @validator('KhoiLuongMuNuoc')
    def check_mu_nuoc(cls, v):
        if v <= 0:
            raise ValueError('KhoiLuongMuNuoc must be greater than 0')
        return v

    @validator('RFID')
    def check_rfid(cls, v):
        if not v:
            raise ValueError('RFID cannot be empty')
        return v

class GiaoDichTSCDRCCreate(BaseModel):
    RFID: str
    TSC: float
    DRC: float

    @validator('TSC', 'DRC')
    def check_do_mu(cls, v):
        if v <= 0:
            raise ValueError('TSC and DRC must be greater than 0')
        return v

    @validator('RFID')
    def check_rfid(cls, v):
        if not v:
            raise ValueError('RFID cannot be empty')
        return v