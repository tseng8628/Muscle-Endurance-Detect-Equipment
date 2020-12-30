function ShowTime(){
		
　          var date=new Date();
			var hours = date.getHours();
			var minutes = date.getMinutes();
			var seconds =date.getSeconds();
			var ampm = hours >= 12 ? 'PM' : 'AM';
			var len_m = minutes.toString().length;
            var len_s = seconds.toString().length;
				
			hours = hours % 12;
			hours = hours ? hours : 12; // the hour '0' should be '12'
			minutes = minutes < 10 ? '0'+minutes : minutes;
				
			
			var strTime = hours + ':' + minutes + ':' + seconds + ' ' + ampm;
			var strTime_1 = hours + ':' + minutes + ':0' + seconds + ' ' + ampm;
			
			if(len_s < 2){
                document.getElementById('showbox').innerHTML = strTime_1; 
            }
            else {
               document.getElementById('showbox').innerHTML = strTime;   
            }
　           setTimeout('ShowTime()',1000);
}