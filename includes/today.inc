<?php

function rbge_cal_today($year, $month, $day){
    
    // if there is no date passed use today
    if(!$year || !$month || !$day){
        $date = new DateTime();        
    }else{
        $date = DateTime::createFromFormat("Y-m-d", "$year-$month-$day");
    }
    
    drupal_set_title($date->format('l jS F Y') . " - All events by start time");
    
    // set up the day
    $day = array();
    $day['start'] = $date->format("Y-m-d") . 'T00:00:00';
    $day['end'] = $date->format("Y-m-d") . 'T23:59:59';
    
    // and any daylight offsetting
    $daylight_saving = date('I');
    
    // get a list of all the events that are currently on or will start today.
    $query = db_select('field_data_field_event_date', 'e');
    $query->addField('e', 'field_event_date_value', 'start');
    $query->addField('e', 'field_event_date_value2', 'end');
    $query->addField('e', 'entity_id', 'event_nid');    
    $query->orderBy('field_event_date_value','DESC');
    
    // starts before and ends after today
    $or = db_or();
       
    // proposed event starts during existing day
    $or->condition( db_and()->condition('e.field_event_date_value', $day['start'], '<=')->condition('e.field_event_date_value2', $day['start'], '>=') );

    // proposed event ends during existing day
    $or->condition( db_and()->condition('e.field_event_date_value', $day['end'], '<=')->condition('e.field_event_date_value2', $day['end'], '>=') );

    // proposed event starts before and finishes after existing day
    $or->condition( db_and()->condition('e.field_event_date_value', $day['start'], '>=')->condition('e.field_event_date_value2', $day['end'], '<=')  );

    // existing event is an all day event that occurs on the same day as the proposed day starts
    $start_day = substr($day['start'], 0, 10) . 'T00:00:00';
    $or->condition( db_and()->condition('e.field_event_date_value', $start_day, '=')->condition('e.field_event_date_value2', $start_day, '='));

    $query->condition($or);
    
    $result =  $query->execute();
      
    // build an array to hold the events by hour
    $events_by_hour = array();
    for($i =0; $i < 24; $i++){
        $events_by_hour[$i] = array();
    }
    
    $events = array();
    $events_by_venue = array();
    $day_start_date = DateTime::createFromFormat("Y-m-d\TH:i:s", $day['start']);

   // dpm(drupal_get_user_timezone());
   // dpm(date('I'));

    foreach($result as $hit){
                
        $event_pack['event'] = entity_metadata_wrapper('node', node_load($hit->event_nid));
        $event_pack['start'] = new DateTime($hit->start);
        $event_pack['end'] = new DateTime($hit->end);
        
        // if the event repeat started before today
        if($event_pack['start']->format('U') < $day_start_date->format('U')){
            $events_by_hour['ongoing'][] = $event_pack;
        }else{
            $hour = $event_pack['start']->format('G') + $daylight_saving; // hour without leading zero plus date
            $events_by_hour[$hour][] = $event_pack;
        }
        $events[$hit->event_nid] = $event_pack;
    
    }
    
    //dpm($events);
  //  dpm($events_by_hour);
    
    $render = array();
    $render['#theme'] = 'item_list';
    $render['#attributes'] = array('class' => 'rbge-day-hours');
    
    foreach($events_by_hour as $hour => $events){
        
        $render['#items'][$hour] = array();
        
        // write in the hour for display
        if(is_numeric($hour)){
            $hour_string = str_pad($hour, 2, '0', STR_PAD_LEFT) . ":00 hrs";
            $hour_anchor = 'hour_' . str_pad($hour, 2, '0', STR_PAD_LEFT) . ":00";
        }else{
            $hour_string = "On Going";
            $hour_anchor = 'hour_on_going';
        }
        
        $render['#items'][$hour]['data'] = "<a name=\"$hour_anchor\"></a><h3>$hour_string</h3>";
        
        foreach($events as $event_data){
            $event = $event_data['event'];
            //$venue = $event->field_event_venue->value();
            //$wrapped_venue = entity_metadata_wrapper('taxonomy_term', $venue->tid);
            $node_view = node_view($event->value(), 'teaser');
            $render['#items'][$hour]['children'][$event->getIdentifier()] =  drupal_render( $node_view  );
        }
        
        // remove hours with no events in before 9am and after 6pm
        if( is_numeric($hour) && !isset($render['#items'][$hour]['children']) && ($hour < 9 || $hour > 18) ){
            unset($render['#items'][$hour]);
        }
                
    }
    
    $output =  array(
        'today' => $render,
      );
      
     //dsm($render);
     // dsm($output);
      
      return $output;

}

