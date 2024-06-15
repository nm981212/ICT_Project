<?php
	$conn = mysqli_connect("localhost", "iot", "pwiot");
#	mysqli_set_charset($conn, "UTF-8");
	mysqli_select_db($conn, "iotdb");
	$query = "select name, date, time, temp, humi, co2_percent from sensor ";
	$result = mysqli_query($conn, $query);

	$data = array(array('SEN_SQL','temp','humi','co2_percent'));

	if($result)
	{
		while($row = mysqli_fetch_array($result))
		{
			array_push($data, array($row['date']."\n".$row['time'], intval($row['temp']),intval($row['humi']) ,intval($row['co2_percent'])));
		}
	}

	$options = array(
			'title' => 'illumination temperature humidity',
			'width' => 1000, 'height' => 400,
			'curveType' => 'function'
			);

?>

<script src="//www.google.com/jsapi"></script>
<script>
var data = <?=json_encode($data) ?>;
var options = <?= json_encode($options) ?>;

google.load('visualization', '1.0', {'packages':['corechart']});

google.setOnLoadCallback(function() {
	var chart = new google.visualization.LineChart(document.querySelector('#chart_div'));
	chart.draw(google.visualization.arrayToDataTable(data), options);
	});
	</script>
<div id="chart_div"></div>

<!DOCTYPE html>
<html>
<head>
	<meta charset = "UTF-8">
	<meta http-equiv = "refresh" content = "30">
	<style type = "text/css">
		.spec{
			text-align:center;
		}
		.con{
			text-align:left;
		}
		</style>
</head>

<body>
	<h1 align = "center">Wildfire Database</h1>
	<div class = "spec">
		# <b>The sensor value description</b>
		<br></br>
	</div>

	<table border = '1' style = "width = 30%" align = "center">
	<tr align = "center">
		<th>ID</th>
		<th>NAME</th>
		<th>DATE</th>
		<th>TIME</th>
		<th>TEMP</th>
		<th>HUMI</th>
		<th>CO2_PERCENT</th>
		<th>ISFIREDETECTED</th>
		<th>LATSTR</th>
		<th>LONSTR</th>
	</tr>

	<?php
		$conn = mysqli_connect("localhost", "iot", "pwiot");
		mysqli_select_db($conn, "iotdb");
		$result = mysqli_query($conn, "select * from sensor");
		while($row = mysqli_fetch_array($result))
		{
			echo "<tr align = center>";
			echo '<th>'.$row['id'].'</td>';
			echo '<th>'.$row['name'].'</td>';
			echo '<th>'.$row['date'].'</td>';
			echo '<th>'.$row['time'].'</td>';
			echo '<th>'.$row['temp'].'</td>';
	      		echo '<th>'.$row['humi'].'</td>';
	      		echo '<th>'.$row['co2_percent'].'</td>';
	      		echo '<th>'.$row['isfireDetected'].'</td>';
			echo '<th>'.$row['latStr'].'</td>';
			echo '<th>'.$row['lonStr'].'</td>';
			echo "</tr>";

		}
			mysqli_close($conn);
	?>
	</table>
</body>
</html>



