 
            var reconnectTimeout = 2000;
            const TOPIC = "Topic/UserID";
            const Feedback_Topic = "Feedback/UserID_State";
            var client = false;
            
            // 用戶端成功連接 broker 時...
            function onConnect(){
                // 確認連接後，才能訂閱主題
                console.log("onConnect then subscribe topic");
                client.subscribe(Feedback_Topic);
            }
            
            // 收到訊息時...
            function onMessageArrived(message){
                console.log("onMessageArrived:"+message.payloadString);
                
                var json = JSON.parse(message.payloadString);
                console.log(json)

                //$("#UserID").html(json['UserID']);
                //var userid = document.getElementById("UserID").innerHTML;
                
                alert("註冊成功！!");
                document.getElementById("mqtt_monitor").innerHTML = message.payloadString;
            }
            
            // 發佈訊息
            function publish_message() {
                var input_text = document.getElementById("mqtt_text");
                var payload = input_text.value;
                var message = new Paho.MQTT.Message(payload);
                message.destinationName = TOPIC;
                client.send(message);
                input_text.value = '';
            }
            
            function init() {
                document.getElementById("mqtt_pub").addEventListener('click', publish_message);
                // 建立 MQTT 用戶端實體. 你必須正確寫上你設置的埠號.
                // ClientId 可以自行指定，提供 MQTT broker 認證用
                client = new Paho.MQTT.Client("ws://localhost:9001/", "myClientId");

                // 指定收到訊息時的處理動作
                client.onMessageArrived = onMessageArrived;

                // 連接 MQTT broker
                client.connect({onSuccess:onConnect});
            }

            //window.addEventListener('load', init, false);
            //document.addEventListener('DOMContentLoaded', init, false);