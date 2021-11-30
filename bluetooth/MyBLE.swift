//
//  MyBLE.swift
//  iosBT
//
//  Created by candy on 2021/11/19.
//

import Foundation
import CoreBluetooth

struct MeterValye: Codable {
    var v: Double
    var trueValue: Double {
        return v * 3.6 / 512
    }
    var u: String
}

class MyBLE: NSObject, ObservableObject , CBCentralManagerDelegate, CBPeripheralDelegate{
    
    let b: CBCentralManager = CBCentralManager()
    @Published var peripherals: [CBPeripheral] = []
    @Published var meterValue = MeterValye(v: 0, u: "--")
    @Published var isconnected = false
    @Published var meterType = "未知仪器"
    override init() {
        super.init()
        b.delegate = self
    }
    func discoverPeriperal() {
        b.scanForPeripherals(withServices: nil, options: nil)
    }
    
    func connect(peripheral: CBPeripheral) {
        if !isconnected {
            b.stopScan()
            b.connect(peripheral)
        }
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        if peripherals.contains(peripheral) ||  peripheral.name == nil  {return}
        
        peripherals.append(peripheral)
    }
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        switch central.state {
        case .poweredOn:
            print("power on")
            discoverPeriperal()
        default:
            print("other")
        }
    }
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        if let _ = error {
            return
        }
        for service in peripheral.services ?? [] {
            peripheral.discoverCharacteristics(nil, for: service)
        }
    }
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?){
        if let _ = error {
            return
        }
        
        for characteristic in service.characteristics ?? [] {
            print("characterID:" + characteristic.uuid.uuidString)
            peripheral.setNotifyValue(true, for: characteristic)
            
        }
        
    }
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        if let _ = error {
            print("error")
            return
        }
        let decoder = JSONDecoder()
        meterValue = try! decoder.decode(MeterValye.self, from: characteristic.value!)
        switch meterValue.u {
        case "v":
            meterType = "电压表"
        case "a":
            meterType = "电流表"
        case "o":
            meterType = "欧姆表"
        default:
            meterType = "未知设备"
        }
    }
    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        if let _ = error {
            return
        }
    }
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        isconnected = true
        print("didConnect" + peripheral.name!)
        print("peripheralID:" + peripheral.identifier.uuidString)
        peripheral.delegate = self
        peripheral.discoverServices(nil)
    }
    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        isconnected = false
    }
    func centralManager(_ central: CBCentralManager, connectionEventDidOccur event: CBConnectionEvent, for peripheral: CBPeripheral) {
        print("connectionEventDidOccur" + peripheral.name!)
    }
}
