import requests
import json
import time
from datetime import datetime
import os
import re

def fetch_weather():
    try:
        # 武汉天气的城市代码为 101200101
        url = "http://t.weather.sojson.com/api/weather/city/101200101"
        response = requests.get(url, timeout=10)
        response.raise_for_status()  # 确保请求成功
        data = response.json()
        
        if data.get("status") == 200:
            forecast = data["data"]["forecast"][0]

            # 提取高温和低温的数值
            high_match = re.search(r'(\d+)', forecast["high"])
            low_match = re.search(r'(\d+)', forecast["low"])
            high_temp = int(high_match.group(1)) if high_match else None
            low_temp = int(low_match.group(1)) if low_match else None

            # 当前温度来自 API 的 wendu 字段
            try:
                api_temp = int(data["data"]["wendu"])
            except Exception:
                # 如果 wendu 解析失败，尝试用高低温均值作为当前温度
                if high_temp is not None and low_temp is not None:
                    api_temp = (high_temp + low_temp) // 2
                elif low_temp is not None:
                    api_temp = low_temp
                else:
                    api_temp = 0  # 兜底值，防止异常

            # 如果 API 当前温度比低温低 2°C 以上，则取 wendu 和 low_temp 中较大值
            if low_temp is not None and api_temp < low_temp - 2:
                current_temp = f"{low_temp}°C"
            else:
                current_temp = f"{api_temp}°C"

            weather_info = {
                "city": data["cityInfo"]["city"],
                "temperature": current_temp,
                "weather": forecast["type"],
                "high": f"{high_temp}°C" if high_temp is not None else forecast["high"],
                "low": f"{low_temp}°C" if low_temp is not None else forecast["low"],
                "update_time": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            }
            
            # 保存到文件
            with open('/mnt/f/My__StudyStack/My_Project/FTB/data/weather.json', 'w', encoding='utf-8') as f:
                json.dump(weather_info, f, ensure_ascii=False, indent=2)
            print("天气更新成功：", weather_info)
        else:
            print("天气数据状态异常：", data)
    except Exception as e:
        print(f"获取天气失败: {e}")

if __name__ == "__main__":
    # 创建数据目录
    os.makedirs('/mnt/f/My__StudyStack/My_Project/FTB/data', exist_ok=True)
    
    # 首次运行立即获取
    fetch_weather()
    
    # 每30分钟更新一次
    while True:
        time.sleep(1800)
        fetch_weather()
