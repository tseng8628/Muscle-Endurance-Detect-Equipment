var time = 0;
		var running = 0;
		var startPause = function () {
			if (running == 0) {
				running = 1;
				increment();
				document.getElementById("startPause").innerHTML = "Pause";
                document.getElementById("start").checked=true;
			}
			else {
				running = 0;
                document.getElementById("stop").checked=true;
				document.getElementById("startPause").innerHTML = "Resume";
			}

			function increment() {
				if (running == 1) {
					var mins = Math.floor(time / 10 / 60);
						var secs = Math.floor(time / 10);
						var tenths = time % 10;
						var timeSecs=secs;
					
				setTimeout(function () {
						time++;
				console.log("timeSecs = "+ timeSecs);			

						if (mins < 10) {
							mins = "0" + mins;
						}
						if (secs < 10) {
							
							secs = "0" + timeSecs;
						}
						if (timeSecs>=60){
							secs = secs %60;
							console.log("reset sec");
							if (secs < 10) {
							
							secs = "0" + secs;
						}

							}
						document.getElementById("aaa").innerHTML = mins + ":" + secs + "." + "" + tenths;
						increment();
					}, 100);
				}
			}
				}
		function resetBtn() {
			running = 0;
			time = 0;
			document.getElementById("startPause").innerHTML = "Start";
            document.getElementById("reset").checked=true;
			document.getElementById("aaa").innerHTML = "00:00.0";
		}