//
//  ContentView.swift
//  bluetooth
//
//  Created by candy on 2021/11/26.
//

import SwiftUI
import CoreBluetooth

struct ContentView: View {
    @State var show = false
    @ObservedObject var cm = MyBLE()
    var body: some View {
        
        ZStack {
            VStack {
                Text(cm.meterType)
                    .frame(width: 200, height: 80)
                    .background(cm.isconnected ? .blue : .gray)
                    .cornerRadius(50)
                    .shadow(radius: 30)
                    .offset(y: 50)
                    .font(.largeTitle)
                    .foregroundColor(.white)
                    .animation(.easeInOut(duration: 1))
                DetailView(number: String(format: "%.3f", cm.meterValue.trueValue), unit: cm.meterValue.u)
                    .position(x: UIScreen.main.bounds.width/2, y: UIScreen.main.bounds.height/2 - 50)
            }
            .blur(radius: show ? 10 : 0)
            ZStack(alignment: .topLeading) {
                Button {
                    withAnimation(.spring()) {
                        show.toggle()
                    }
                } label: {
                    Image(systemName: "list.bullet.rectangle.portrait")
                        .padding(.leading, 30)
                        .font(.largeTitle)
                        .frame(width: 120, height: 60)
                        .background(.white)
                        .cornerRadius(30)
                        .shadow(color: .gray, radius: 10, x: 0, y: 10)
                }
                .offset(x: -UIScreen.main.bounds.width/2 + 30, y: -200)
            }
            .blur(radius: show ? 10 : 0)
            DevicesView(show: $show, myBLE: cm)
        }
        
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
.previewInterfaceOrientation(.portrait)
    }
}

struct DevicesView: View {
    @Binding var show: Bool
    @State private var viewState = CGSize.zero
    var myBLE: MyBLE
    var body: some View {
        VStack(alignment: .leading, spacing: 20) {
            Text("蓝牙列表")
            ForEach(myBLE.peripherals, id: \.self) { item in
                Button {
                    withAnimation(.spring()) {
                        show.toggle()
                    }
                    myBLE.connect(peripheral: item)
                } label: {
                    Label(item.name!, systemImage: "barometer")
                        .font(.title3)
                }
            }
            Spacer()
            
        }
        .padding(.top, 70)
        .frame(minWidth: 0, maxWidth: .infinity)
        .background(.white)
        .cornerRadius(30)
        
        .offset(x: show ? 0 : -UIScreen.main.bounds.width - 10)
        .offset(x: viewState.width, y: viewState.height)
        .padding(.trailing, 60)
        .shadow(radius: 20)
        .rotation3DEffect(.degrees(show ? 0 : 60), axis: (x: 0, y: 10, z: 0))
        .onTapGesture(perform: {
            withAnimation(.spring()) {
                show.toggle()
            }
        })
        .gesture(DragGesture()
            .onChanged({ value in
                viewState = value.translation
            })
                    .onEnded({ value in
            withAnimation(.spring()) {
                if value.translation.width < viewState.width {
                    show = false
                }
                viewState = CGSize.zero
            }
            
        })
        )
    }
}
struct DeviceRow: View {
    var deviceName: String
    var body: some View {
        
        Label(deviceName, systemImage:"link")
                .font(.title)
        
    }
}
