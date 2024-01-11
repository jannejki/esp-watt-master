#ifndef WEBSITE_H
#define WEBSITE_H

// note R"KEYWORD( html page code )KEYWORD";
// again I hate strings, so char is it and this method let's us write naturally

char PAGE_MAIN[] PROGMEM = R"=====(

<!DOCTYPE html>
<html lang="en" class="js-focus-visible">

<title>Web Page Update Demo</title>

  <body style="background-color: #efefef" onload="process()">
  
    <header>
    <p>Hello world</p>
    </header>
  
    <main class="container" style="margin-top:70px">
   
  </main>
  
  </body>

</html>



)=====";
#endif  // WEBSITE_H