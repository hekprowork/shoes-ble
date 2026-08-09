import asyncio
import logging
import struct
from bleak import BleakScanner, BleakClient

# 根據你的 ESP32 程式碼設定 UUID（注意：Bleak 內部會自動轉為小寫比較）
SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0"
CHARACTERISTIC_UUID = "abcdef01-1234-5678-1234-56789abcdef0"
DEVICE_NAME = "ESP32-MPU6050-Tester"

# 設定 Logging，方便除錯
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

def notification_handler(sender: int, data: bytearray):
    """
    當 ESP32 發送 Notify 時，會自動觸發這個函式
    """
    try:
        # 確認資料長度是否為 16 Bytes
        if len(data) == 16:
            # 解開 16 bytes 二進制封包 (<HHhhhhhh)
            # < : Little Endian
            # H : unsigned short (uint16_t)
            # h : signed short (int16_t)
            fsr1, fsr2, raw_acc_x, raw_acc_y, raw_acc_z, raw_gyro_x, raw_gyro_y, raw_gyro_z = struct.unpack('<HHhhhhhh', data)
            
            # 轉換回浮點數 (因為在 ESP32 上乘了 100)
            acc_x = raw_acc_x / 100.0
            acc_y = raw_acc_y / 100.0
            acc_z = raw_acc_z / 100.0
            gyro_x = raw_gyro_x / 100.0
            gyro_y = raw_gyro_y / 100.0
            gyro_z = raw_gyro_z / 100.0
            
            print(f"[收到數據] FSR1: {fsr1}, FSR2: {fsr2} | ACC: {acc_x:.2f}, {acc_y:.2f}, {acc_z:.2f} | GYRO: {gyro_x:.2f}, {gyro_y:.2f}, {gyro_z:.2f}")
        else:
            print(f"[異常資料] 預期 16 Bytes，收到 {len(data)} Bytes。內容: {data.hex()}")
    except Exception as e:
        print(f"解碼失敗: {e}")

async def main():
    print(f"正在搜尋藍牙裝置: {DEVICE_NAME} ...")
   
     
    # 1. 掃描並尋找特定的 ESP32 裝置
    devices_and_adv = await BleakScanner.discover(timeout=5.0, return_adv=True)
    device = None
    
    # 尋找廣播了 SERVICE_UUID 的裝置
    for address, (devi, adv_data) in devices_and_adv.items():
        advertised_uuids = [uuid.lower() for uuid in adv_data.service_uuids]
        if SERVICE_UUID.lower() in advertised_uuids:
            device = devi
            break

    if not device:
        print(f"❌ 找不到名稱為 '{DEVICE_NAME}' 的裝置，請確認 ESP32 是否已開機並在廣播中。")
        return

    print(f"✅ 找到裝置！ MAC地址/UUID: {device.address}")
    print(f"正在嘗試連線到 {device.address}...")

    # 2. 連線到 ESP32
    async with BleakClient(device) as client:
        if client.is_connected:
            print("🎉 連線成功！")
            
            # 3. 開始訂閱（Notify）特徵值
            print(f"正在訂閱特徵值: {CHARACTERISTIC_UUID}")
            await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
            
            print("開始接收感測器資料，按下 Ctrl+C 可以停止...\n")
            
            # 保持連線，讓程式持續等待 Notify
            while True:
                await asyncio.sleep(1)
        else:
            print("❌ 連線失敗。")

if __name__ == "__main__":
    try:
        # 執行非同步主程式
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n使用者停止程式，正在中斷連線...")