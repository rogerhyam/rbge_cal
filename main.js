jQuery(document).ready(function($) {
  
   // work through the localstorage and hide the divs they don't want to see
  var aState = localStorage.getItem('availability-state');
  if(aState){
      
      aState = JSON.parse(aState);
      
      for (var i=0; i<aState.length; i++){
           
           var venue_class = aState[i].trim();

            if(venue_class.indexOf('.rbge-cal-venue-') == 0){

                 // hide all the children elements
                 $(venue_class).hide("fast");

                 // set the switch correctly
                 var venue_id = venue_class.replace('-child', '');
                 venue_id = venue_id.replace('.', '#');
                 $('tr' + venue_id + ' span.rbge-cal-venue-switch-on').show("fast");
                 $('tr' + venue_id + ' span.rbge-cal-venue-switch-off').hide("fast");

            }
      }
      
  }
  
  // on the today page we have a jump spot.
  if($( "#today-jump-date" ).length){
      $('#tabs-wrapper').append($( "#today-jump-date" ));
      $( "#today-jump-date" ).datepicker({
        dateFormat: "yy/mm/dd"
      });
      $( "#today-jump-date" ).change(function(){
          window.location = '/rbge/today/' + $( "#today-jump-date" ).val();
          $( "#today-jump-date" ).hide();
      });
  }
  
  // event view popup - UP
  $('.venue_repeats').mouseover( function(event){
      var repeat_div = $(this);
      var dialogue = repeat_div.find('.venue_repeat_dialogue');
      dialogue.show();
  });

  // event view popup - DOWN
  $('.venue_repeats').mouseout( function(event){
      $(this).find('.venue_repeat_dialogue').hide();
  });

  // add an on switch handler to display children
  $('.rbge-cal-venue-switch-on').click(function(){

      // get the id of the grand parent
      var grand_id = $(this).parent().parent().attr('id');
      var venue_class = '.' + grand_id + '-child'
      
      // show all the children elements
      $(venue_class).show("fast");
      
      // hide the on switch
      $(this).parent().find('.rbge-cal-venue-switch-on').hide("fast");
      
      // show the off switch
      $(this).parent().find('.rbge-cal-venue-switch-off').show("fast");
      
      // remove this from the local storage
      var aState = localStorage.getItem('availability-state');
      if(!aState) aState = []; else aState = JSON.parse(aState);
      
      for (var i=aState.length-1; i>=0; i--) {
          if (aState[i] === venue_class) {
              aState.splice(i, 1);
              break;
          }
      }
      localStorage.setItem('availability-state', JSON.stringify(aState));
      
  });
  
  // add an on switch handler to display children
  $('.rbge-cal-venue-switch-off').click(function(){

      // get the id of the grand parent
      var grand_id = $(this).parent().parent().attr('id');
      var venue_class = '.' + grand_id + '-child'
      
      // hide all the children elements
      $(venue_class).hide("fast");
      
      // hide the off switch
      $(this).parent().find('.rbge-cal-venue-switch-off').hide("fast");
      
      // show the on switch
      $(this).parent().find('.rbge-cal-venue-switch-on').show("fast");
      
      // add this to the hidden list in the localstorage
      var aState = localStorage.getItem('availability-state');
      if(!aState) aState = []; else aState = JSON.parse(aState);
      aState.push(venue_class);
      localStorage.setItem('availability-state', JSON.stringify(aState));
      
  });
  
   // bubble to help click the right place
   $('tr.rbge-cal-venue-row div.period-heading').mousemove(function(event){
       
       var period_div = $(this);
       var cursor_time = $('#cursor-time');
       var start_dt = period_div.data('start');    
       var seconds = mousePositionInSeconds(period_div, event);
       
       // are we looking at a day or an hour time period
       var hour;
       var mins;
       if(start_dt.match(/T00:00:00\+/i)){
           
           hour = Math.floor(seconds/60/60);
           mins = Math.round((seconds/60) - (hour * 60));
        
       }else{
           var matches = start_dt.match(/T([0-9]+):/i);
           hour = matches[1];
           mins = Math.round(seconds/60);
       }
       
       var pad = "00";
       mins = "" + (Math.floor(mins / 15) * 15);
       mins = pad.substring(mins.length) + mins;
       hour = "" + hour;
       hour = pad.substring(hour.length) + hour;
       cursor_time.html(hour + ":" + mins);
       cursor_time.show();
       cursor_time.css({color: 'black', background: 'white', opacity: 0.8, 'z-index': 1000, padding: '2px'});
       cursor_time.appendTo(period_div);

       // move the display to keep up with the mouse.
       var left = event.pageX + 8;
       var top = event.pageY + 8;
       cursor_time.offset({ top: top, left: left });

   });

   // we need to hide the cursor time display when 
   // scrolling or it is left halfway up/down the page
   $(window).scroll(function(event){
       if($('#cursor-time').length){
           $('#cursor-time').hide();
       }
   });
   
   $('tr.rbge-cal-venue-row div.period-heading').mouseout(function(event){
       $('#cursor-time').hide();
   });
  
  // book now click handler
   $('tr.rbge-cal-venue-row div.period-heading').click(function(event){
       
       // calculate the start point of the div they clicked on 
       // plus the number of extra seconds beyond that.
       var period_div = $(this);
       var start_dt = period_div.data('start');
       //var duration_sec = period_div.data('duration');
       //var click_x = event.pageX - period_div.offset().left;
       //var plus_sec = duration_sec * Math.round((click_x / period_div.width()) * 100) / 100;
       var plus_sec = mousePositionInSeconds(period_div, event)
       
       // work out the venue row they clicked on
       var venue_tr = $(period_div.parents('tr.rbge-cal-venue-row')[0]);
       var venue_tid = venue_tr.data('venue-tid');

       // build a url and send them there
       var url = '/node/add/event?date=' + encodeURIComponent(start_dt) + '&plus_seconds=' + plus_sec + '&venue_tid=' + venue_tid;
       window.location = url;
       
   });
   
   function mousePositionInSeconds(period_div, event){
       
        var start_dt = period_div.data('start');
        var duration_sec = period_div.data('duration');
        var click_x = event.pageX - period_div.offset().left;
        var plus_sec = duration_sec * Math.round((click_x / period_div.width()) * 100) / 100;
        return plus_sec;
   }
  
  // bit of prettiness for the dates display - put a border around the field-items when they have 
  // multiple dates
  $('div.field.field-name-field-event-date.field-type-date div.field-item.odd').parent().css('border', 'solid 1px #cccccc');
  $('div.field.field-name-field-event-date.field-type-date div.field-item.odd').parent().css('font-weight', 'normal');
  
  
  // the today page has dates on that need to highlighted if the occur now
  // in fact this could happen anywhere.
 
  function highlightCurrentEvents(){
      
      var nowMilli = Date.now();
      
      // work through all the things that might want
      // highlighting or unhighlighting
      $('.rbge-highlight-if-now').each(function(){
          
          var el = $(this);

          // do nothing if we don't have a start and end time set
          if(!el.data('start-milli')) return;
          if(!el.data('end-milli')) return;
          
          var startMilli = el.data('start-milli');
          var endMilli = el.data('end-milli');
                 
          if(nowMilli >= startMilli && nowMilli <= endMilli){
              el.addClass('rbge-highlight-as-now');
          }else{
              el.removeClass('rbge-highlight-as-now');
          }
          
      });
      
      // run myself again in 2 minutes - time flies!
      setTimeout(highlightCurrentEvents, 2*60*1000);
      
  }
  highlightCurrentEvents(); // kick it off on page load
  
});


// standard closure
/*
(function ($) {
    
}(jQuery));
*/