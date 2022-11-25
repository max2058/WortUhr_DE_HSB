
#ifndef HTML_h
#define HTML_h

const char LoginHTML[] =  R"(<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    
    <style>
    
      .background {
        background-color: rgb(190, 190, 190);
        font-family: Arial;
      }
      
      .header {
        text-align: center;
        height: 50px;
        width: 100%;
        margin-top: 60px;
      }

      .header2 {
        text-align: center;
        height: 40px;
        width: 100%;
        margin-top: 20px;
      }


      .headline1{
        text-align: center;
        padding-right: 160px;
      }

      .headline2{
        text-align: center;
        padding-right: 190px;
      }

      .headline3{
        text-align: center;
        padding-right: 250px;
      }

      .headline4{
        text-align: center;
        padding-right: 140px;
      }
      
      .container {
        text-align: center;
        border-radius: 6px;
        margin-left: 10px;
        margin-right: 10px;
        padding-top: 20px;
        padding-bottom: 10px;
        background-color: rgb(150, 150, 150);
      }
      
      .ssid {
        height: 45px;
      }
      
      .password {
        height: 70px;
      }

      .slider {
        height: 60px;
      }

      .nightmode {
        height: 30px;
        padding-right: 100px; 
      }

      .display {
        height: 45px;
      }

      .refresh{
        height: 50px;
      }

      .save{
        height: 40px;
      }
      
      .info {
        height: 60px;
        margin-top: 25px;
        margin-right: 20px;
        margin-left: 20px;  
        text-align: center;
      }

      label[for=rgbred]{
        border-radius: 3px;
        background-color: red;
        padding: 4px 14px 4px 14px;
        font-size: medium;
      }

      label[for=rgbgreen]{
        border-radius: 3px;
        background-color: green;
        padding: 4px 7px 4px 7px;
        font-size: medium;
      }

      label[for=rgbblue]{
        border-radius: 3px;
        background-color: blue;
        padding: 4px 9px 4px 9px;
        font-size: medium;
      }     
      
      input[type=text],[type=password] {
        height: 25px;
        width: 300px;
        border-radius: 3px;
        border: 1px solid rgb(10, 10, 10);
        margin-top: 3px;
      }

      input[type=range]{
        width: 230px;
      }
      
      input[type=submit] {
        height: 30px;
        width: 180px;
        border-radius: 3px;
        border: 1px solid rgb(10, 10, 10);
        background-color: rgb(210, 210, 210);
        cursor: pointer;   
      }
      
    </style>
    
    <title>WortUhr</title>
</head>
<body class="background">

    <div class="header">
        <h2>WortUhr / WordClock</h2>
    </div>

    <div class="container">

    <div class="header2">
        <h3>Version 1.3.0.5 - 2022-11-23 - Auerbach Maximilian</h3>
    </div>
    
        <h2>Erstinbetriebnahme</h2>

        <form action="/refresh" method="post">

            <div class="headline1">
                <h3>Farbeinstellung</h3>
            </div>

            <div class="slider">
                <label for="rgbred">ROT</label>
                <input type="range" min="0" max="255" name="rgbred" id="rgbred" step="1" value="%%RED%%">
            </div>

            <div class="slider">
                <label for="rgbgreen">GRÜN</label>
                <input type="range" min="0" max="255" name="rgbgreen" id="rgbgreen" step="1" value="%%GREEN%%">
            </div>

            <div class="slider">
                <label for="rgbblue">BLAU</label>
                <input type="range" min="0" max="255" name="rgbblue" id="rgbblue" step="1" value="%%BLUE%%">
            </div>

            <div class="refresh">
                <input type="submit" value="AKTUALISIEREN">
            </div>


        </form>

        <form action="/save" method="post">



            <div class="headline4">
                <h3>Aussprachemodus</h3>
            </div>
            <div> 
              <input type="radio" name="Clockmode1" id="Clockmode1" value="1" checked>
              <label for="Clockmode1">Dreiviertel Drei</label>
        
              <input type="radio" name="Clockmode1" id="Clockmode1" value="0">
              <label for="Clockmode1">Viertel vor Drei</label> 
            </div>

            <div class="headline2">
                <h3>Nachtmodus</h3>
            </div>

            <div class="nightmode">
                <label for="enablenightmode">Nachtmodus verwenden</label>
                <input type="checkbox" name="enablenightmode" id="enablenightmode">              
            </div>

            <div class="display">
                <input type="text" name="displayoff" id="displayoff" placeholder=" Uhr ausschalten um XX Uhr">
            </div>

            <div class="display">
                <input type="text" name="displayon" id="displayon" placeholder=" Uhr einschalten um XX Uhr">
            </div>

            <div class="headline3">
                <h3>WLAN</h3>
            </div>

            <div class="ssid">
                <input type="text" name="ssid" id="ssid" placeholder=" SSID">
            </div>
                
            <div class="password">
                <input type="password" name="password" id="password" placeholder=" Passwort">
            </div>
                
            <div class="save">
                <input type="submit" value="SPEICHERN & BEENDEN">
            </div>

            <div class="info">
                Die Daten werden unverschlüsselt an die WortUhr übertragen.
            </div>
    
        </form>

    </div>



</body>
</html>)";


const char NotFoundHTML[] =  R"(<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
      .background{
        background-color: rgb(190, 190, 190);
        font-family: Arial;
      }
      .header{
        text-align: center;
        height: 50px;
        width: 100%;
        margin-top: 60px;
      }
    </style>
    <title>404 Not Found</title>
</head>
<body class="background">

    <div class="header">
        <h2>404 Not Found</h2>
        <p>Die WortUhr ist verwirrt und kann deine Anfrage leider nicht beantworten.</p>
    </div>

</body>
</html>)";


const char SavedHTML[] =  R"(<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
      .background{
        background-color: rgb(190, 190, 190);
        font-family: Arial;
      }

      .header{
        text-align: center;
        height: 50px;
        width: 100%;
        margin-top: 60px;
      }
    </style>
    <title>Login Saved</title>
</head>
<body class="background">

    <div class="header">
        <h2>Fast geschafft!</h2>
        <p>Du musst die WortUhr nur noch neu starten.</p>
    </div>

</body>
</html>)";

#endif
