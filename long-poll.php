<?php
// Longpoll script, Thijs Kaper, 11/4/2016
// Usage: call with parameters user+code to set the code
//        call with just user parameter to longpoll for the code  (max 60 seconds)
// Intendend purpose; create android app to wait for SMS authorization code,
// let it write the code to this web-service, and use "curl" from a shell
// script to wait for this sms code, and use it in the process which needed the code.
// Or more specific; VPN logon with sms authentication...
//
// 11/11/2018 - So... originally this script was for passing on SMS codes.
// Now let's try a new use. Having an IOT device (esp8266) polling for data to be fed to it.
// The esp8266 will hang in a polling loop, and as soon as it receives some data, it can act on
// it. In this case, we will use it to turn on/off an alarm rotating flash light to indicate
// platform or application failures in a kubernetes server cluster.
// We needed an internet service for this, as internally we can not onboard just any device on
// the network, due to strict security policies. The IOT device is therefore connected to 
// "normal" external only internet. And our platform monitoring tools will send a notification
// to the internet long-poll gateway.
// 
// Note: there's no security whatsoever in this script. Use at your own risk, or extend it's function.
// The script currently uses text files in the same folder as the script to communicate between
// receiver and sender. So you need folder write access for this process. You might want to update this
// to write and read those files in another folder. But I did not see any real harm in this for now.
// 
// If the long-poll gives a time out, the client will get a 404 status code, and no message.
 
header("Content-type: text/plain");
 
$user = isset($_GET['user']) ? $_GET['user'] : "";
if(!$user) {
   header($_SERVER["SERVER_PROTOCOL"]." 400 Missing user parameter...", true, 400);
   return;
}
 
// Construct file name from user parameter. Clean up unwanted characters to prevent hacking.
$filename="data_" . preg_replace("/[^a-zA-Z0-9]+/", "_", $user) . ".txt";
$filename_poller="data_" . preg_replace("/[^a-zA-Z0-9]+/", "_", $user) . ".poll";
 
$code = isset($_GET['code']) ? $_GET['code'] : "";
if($code) {
   // store data
   file_put_contents($filename, $code);
   header($_SERVER["SERVER_PROTOCOL"]." 204 Data stored...", true, 204); 
   return;
}
 
// When adding parameter status, we do check if we have seen the client lately
if (isset($_GET['status'])) {
   if (file_exists($filename_poller)) {
      if (time()-filemtime($filename_poller) > 68) {
         echo "OFFLINE " . date("F d Y H:i:s.", filemtime($filename_poller));
      } else {
         echo "ONLINE " . date("F d Y H:i:s.", filemtime($filename_poller));
      }
      return;
   } else {
      echo "OFFLINE";
      return;
   }
}
 
// No code parameter? Go into long poll mode for reading the data...
 
// Mark a file to indicate polling is active
// I'm behind an nginx proxy, therefore using header X-Real-IP to store client ip in the status file.
// You might want to change that to $_SERVER['REMOTE_ADDR'].
file_put_contents($filename_poller, $_SERVER['HTTP_X_REAL_IP'] . " " . $_SERVER['HTTP_USER_AGENT']);
 
if (file_exists($filename) && time()-filemtime($filename) > 600) {
   // Datafiles older than 10 minutes will be removed. They probably do not belong to this session.
   unlink($filename);
}
 
// Long poll process may last max 64 seconds. Note: this command does not have effect on hardened php servers.
set_time_limit(64);
 
// Wait for existence of the code file, and wait max 60 seconds.
$expireseconds = time() + 60;
while (!file_exists($filename) && time() < $expireseconds) {
   // sleep 0.2 seconds
   usleep(200000);
}
 
if (!file_exists($filename) && time() >= $expireseconds) {
   header($_SERVER["SERVER_PROTOCOL"]." 404 Data not found within reasonable time...", true, 404); 
   return;
}
 
// send back the file contents, and remove the file
echo file_get_contents($filename);
unlink($filename);
 
// exit script to prevent additional newlines after output
return;
?>
