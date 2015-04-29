<!DOCTYPE html> 
<?php 
/* 
Page to display heat exchanger temperatures via linux socket
Jeff Glancy
2015-04-01
*/

?>

<html>
  <head>
    <meta charset="UTF-8">
	<title>Capstone Heat Exchanger</title>
    <meta name="description" content="Capstone Heat Exchanger">
	<meta id="viewport" name="viewport" content="width=device-width; initial-scale=0.6; maximum-scale=1.2;" />
    <link rel="stylesheet" href="stylesheets/iphone.css" />
	<script type="text/javascript" src="jquery-2.1.1.min.js"></script>
	<script type="text/javascript">


//only use for Get variables!!
function loadTempVariable(variable) {
	
	var temp, r, g, b;
	temp = 75.0;

	//call for JSON data
	$.ajax({
		url: "socket.php",
		data: "Variable=" + variable + "&Value=0",
		type: "POST",
		dataType: "json"
		})
		.done(function (jsonData) { 
			var temp = parseFloat(jsonData.Value);
//			$('#Variable').val(jsonData.Variable);
//			$('#Variable').val(jsonData.Value);
//			temp = parseFloat(jsonData.Value);
			$('#' + variable).html(jsonData.String);
			
			if (temp >= 70.0)
			{
				r = Math.round((temp - 70.0) * 5);
				if (r > 255)
					r = 255;
				g = Math.round((95.0 - temp) * 10);
				if (g < 0)
					g = 0;
				b = 0;
			}
			else
			{
				r = 0;
				g = Math.round((temp - 45.0) * 10);
				if (g < 0)
					g = 0;
				b = Math.round((70.0 - temp) * 5);
				if (b > 255)
					b = 255;
			}
			$('#' + variable).css("color","rgb(" + r + "," + g + "," + b + ")");
	
		});	
}

function loadAll() {

	//load all temp data and color it
	loadTempVariable('GetTemp1');
	loadTempVariable('GetTemp2');
	loadTempVariable('GetTemp3');
	loadTempVariable('GetTemp4');
	loadTempVariable('GetTemp5');
	loadTempVariable('GetTemp6');
	loadTempVariable('GetTemp7');
	loadTempVariable('GetTemp8');
	loadTempVariable('GetTemp9');
	loadTempVariable('GetTemp10');
}
	
$( loadAll() );

// refresh interval
setInterval(loadAll, 2500);
	
    </script>
  </head>

  <body id="normal">
     <div id="header">
	<h1>Heat Exchanger Temperatures</h1>
	<!--<a href="" id="backButton">Index</a>-->
	<!--<a href="javascript: loadAll();" class="nav Action">Refresh</a>-->
    </div>

	<h4>Current Data</h4>

<table>
	<tr>
		<td width="50">&nbsp;</td>
		<td width="150">&nbsp;</td>
		<td width="50">&nbsp;</td>
		<td width="150">&nbsp;</td>
		<td width="50">&nbsp;</td>
		<td width="150">&nbsp;</td>
		<td width="50">&nbsp;</td>
		<td width="150">&nbsp;</td>
		<td width="50">&nbsp;</td>
		<td width="150">&nbsp;</td>
		<td width="50">&nbsp;</td>
	</tr>
	<tr>
		<td>&nbsp;</td>
		<td style="background-color:white" title="Left">Left</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td style="background-color:white" title="Inner Pipe Temperatures:">Inner Pipe Temperatures:</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td style="background-color:white" title="Right">Right</td>
		<td>&nbsp;</td>
	</tr>
	<tr>
		<td>&nbsp;</td>
		<td id="GetTemp1" style="background-color:white" title="Temp 1">&nbsp;</td>
		<td>&nbsp;</td>
		<td id="GetTemp2" style="background-color:white" title="Temp 2">&nbsp;</td>
		<td>&nbsp;</td>
		<td id="GetTemp3" style="background-color:white" title="Temp 3">&nbsp;</td>
		<td>&nbsp;</td>
		<td id="GetTemp4" style="background-color:white" title="Temp 4">&nbsp;</td>
		<td>&nbsp;</td>
		<td id="GetTemp5" style="background-color:white" title="Temp 5">&nbsp;</td>
		<td>&nbsp;</td>
	</tr>
	<tr>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
	</tr>
	<tr>
		<td>&nbsp;</td>
		<td style="background-color:white" title="Outlet">Outlet</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td style="background-color:white" title="Outer Pipe Temperatures:">Outer Pipe Temperatures:</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td style="background-color:white" title="Inlet">Inlet</td>
		<td>&nbsp;</td>
	</tr>
	<tr>
		<td>&nbsp;</td>
		<td id="GetTemp6" style="background-color:white" title="Temp 6">&nbsp;</td>
		<td>&nbsp;</td>
		<td id="GetTemp7" style="background-color:white" title="Temp 7">&nbsp;</td>
		<td>&nbsp;</td>
		<td id="GetTemp8" style="background-color:white" title="Temp 8">&nbsp;</td>
		<td>&nbsp;</td>
		<td id="GetTemp9" style="background-color:white" title="Temp 9">&nbsp;</td>
		<td>&nbsp;</td>
		<td id="GetTemp10" style="background-color:white" title="Temp 10">&nbsp;</td>
		<td>&nbsp;</td>
	</tr>
	<tr>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
	</tr>
</table>

	<h4>&nbsp;</h4>
	
  </body>
</html>
