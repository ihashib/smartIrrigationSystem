<?php
class dht11{
 public $link='';
 function __construct($pump_stat, $moist_thresh, $temp, $hum, $moist){
  $this->connect();
  $this->storeInDB($pump_stat, $moist_thresh, $temp, $hum, $moist);
 }
 
 function connect(){
  $this->link = mysqli_connect('localhost','root','') or die('Cannot connect to the DB');
  mysqli_select_db($this->link,'smart_irrigation') or die('Cannot select the DB');
 }
 
 function storeInDB($pump_stat, $moist_thresh, $temp, $hum, $moist){
  $query = "insert into system_data set pump_stat='".$pump_stat."', moist_thresh='".$moist_thresh."',  hum='".$hum."', temp='".$temp."', moist='".$moist."'";
  $result = mysqli_query($this->link,$query) or die('Errant query:  '.$query);
 }
 
}
if($_GET['pump_stat'] != '' and $_GET['moist_thresh'] != '' and $_GET['temp'] != '' and  $_GET['hum'] != '' and  $_GET['moist'] != ''){
 $dht11=new dht11($_GET['pump_stat'],$_GET['moist_thresh'],$_GET['temp'],$_GET['hum'],$_GET['moist']);
}


?>
