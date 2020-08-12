import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

ListView {
    id: list
    verticalLayoutDirection: ListView.BottomToTop
    
    required model
    
    delegate: RowLayout {
        id: root
        width: ListView.view.width
        
        // placeholder
        Item {
            Layout.preferredWidth: root.layoutDirection === Qt.LeftToRight ? 5 : 10
        }
        
//         Avatar {
//             id: avatar
//             visible: !sentByMe
//             avatarUrl: root.avatarUrl
//             Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
//             name: root.senderName
//             Layout.preferredHeight: Kirigami.Units.gridUnit * 2.2
//             Layout.preferredWidth: Kirigami.Units.gridUnit * 2.2
//         }
        
        Item {
            Layout.preferredWidth: content.width + 16
            Layout.preferredHeight: content.height + 16
            
            Rectangle {
                id: messageBubble
                anchors.fill: parent
                
                color: "blue"
            }
            
            ColumnLayout {
                id: content
                spacing: 5
                anchors.centerIn: parent
                
                Label {
                    text: model.sender
                }
                
                Label {
                    text: model.text
                    wrapMode: Text.Wrap
                    Layout.maximumWidth: root.width
                }
                
                // message meta data: date, deliveryState
                RowLayout {
                    Layout.bottomMargin: -4
                    
                    Label {
                        id: dateLabel
                        text: model.date
                    }
                    
//                     Icon {
//                         source: "edit-symbolic"
//                         visible: model.correction
//                         Layout.preferredHeight: 10
//                         Layout.preferredWidth: 10
//                     }
                }
            }
        }
        
        Item {
            Layout.fillWidth: true
        }
    }
}
