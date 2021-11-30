//
//  DetailView.swift
//  bluetooth
//
//  Created by candy on 2021/11/27.
//

import SwiftUI

struct DetailView: View {
    var number: String
    var unit: String
    var body: some View {
        HStack {
            Text(number)
                .fontWeight(.bold)
                .frame(width: 130)
                .font(.largeTitle)
                .foregroundColor(.mint)
            Text(unit)
                .font(.largeTitle)
                .fontWeight(.heavy)
                .foregroundColor(Color.pink)
        }
    }
}

struct DetailView_Previews: PreviewProvider {
    static var previews: some View {
        DetailView(number: "17.2", unit: "V")
    }
}

