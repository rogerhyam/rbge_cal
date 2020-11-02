<?php

function get_venue_event_tree($period){
    
    $venue_voc = taxonomy_vocabulary_machine_name_load('venues');

    // load the whole vocabulary from root and load the entities
    $venues = taxonomy_get_tree($venue_voc->vid, 0, NULL, true);

    // convert it into an array by tid
    $venues_by_tid = array();
    foreach($venues as $venue){
       $venues_by_tid[$venue->tid] = $venue;
    }

    // build that into a tree structure
    $roots = array();
    foreach($venues_by_tid as $venue){
       if($venue->parents[0] == 0){
           $roots[] = $venue;
       }else{
           $venues_by_tid[$venue->parents[0]]->children[] = $venue;
       }
    }

    // add in all the events for this date to the tree structure
    add_events($venues_by_tid, $period);

    // sort the roots by weight
    uasort($roots, 'sort_by_weight');
    
    // the ids of the roots are stored in the -1 of the array
    $venues_by_tid[-1] = $roots;
    
    
    return $venues_by_tid;
    
}

function add_events(&$venues, $period){

/*
    // get a list of all the events that are currently on or will start today.
    $query = db_select('field_data_field_event_date', 'e');
    $query->addField('e', 'field_event_date_value', 'start');
    $query->addField('e', 'field_event_date_value2', 'end');
    $query->addField('e', 'entity_id', 'event_nid');    
    $query->orderBy('field_event_date_value','ASC');
    
    // starts before and ends after today
    $or = db_or();
       
    // proposed event starts during existing day
    $or->condition( db_and()->condition('e.field_event_date_value', $period['start'], '>=')->condition('e.field_event_date_value', $period['end'], '<=') );

    // proposed event ends during existing day
    $or->condition( db_and()->condition('e.field_event_date_value2', $period['start'], '>=')->condition('e.field_event_date_value2', $period['end'], '<=') );

    // proposed event starts before and finishes after existing day
    $or->condition( db_and()->condition('e.field_event_date_value', $period['start'], '<')->condition('e.field_event_date_value2', $period['end'], '>')  );

    // existing event is an all day event that occurs on the same day as the proposed day starts
    $start_day = substr($period['start'], 0, 10) . 'T00:00:00';
    $or->condition( db_and()->condition('e.field_event_date_value', $start_day, '=')->condition('e.field_event_date_value2', $start_day, '='));

    $query->condition($or);
    
    $result =  $query->execute();
    
*/ 
 
    $date_start = $period['start'];
    $date_end = $period['end'];
   
    // special case for all day events - just for comparison move the end time to just before midnight
    // fixme? - there is a flag in the field for all day events..
    if($period['start'] == $period['end'] && preg_match('/T00:00:00$/', $period['start'])){
        $period['end'] = substr($period['end'], 0, 10) . 'T23:59:59';
    }
    $start_day = substr($period['start'], 0, 10) . 'T00:00:00';

    $sql = "SELECT
        e.field_event_date_value AS start,
        e.field_event_date_value2 AS end,
        e.entity_id AS event_nid
    FROM field_data_field_event_date e INNER JOIN field_data_field_event_venue v ON v.entity_id = e.entity_id
    WHERE
    (
        (
                 # Does the start time fall within another event - ok for start time to be same as end time of other event
                (CAST(e.field_event_date_value AS DATETIME) <= CAST('$date_start' AS DATETIME)) AND (CAST(e.field_event_date_value2 AS DATETIME) > CAST('$date_start' AS DATETIME))
        )OR(
                 # Does the end time fall within another event - ok for end time to be same as start time of other event
                (CAST(e.field_event_date_value AS DATETIME) < CAST('$date_end' AS DATETIME)) AND (CAST(e.field_event_date_value2 AS DATETIME) >= CAST('$date_end' AS DATETIME))
        )OR(
                 # Does it start before and end after another event
                (CAST(e.field_event_date_value AS DATETIME) >= CAST('$date_start' AS DATETIME)) AND (CAST(e.field_event_date_value2 AS DATETIME) <= CAST('$date_end' AS DATETIME))
        )OR(
                 # is it an all day event that matches
                (CAST(e.field_event_date_value AS DATETIME) = CAST('$start_day' AS DATETIME)) AND (CAST(e.field_event_date_value2 AS DATETIME) = CAST('$start_day' AS DATETIME))
        )
    )";
    $result = db_query($sql);
    
    // work through them and load them
    $londonTz = new DateTimeZone('Europe/London');
    $utcTz = new DateTimeZone('UTC');
    foreach($result as $hit){
    
        // timezone to hits as it is UTC in the DB
        $start_time = new DateTime($hit->start, $utcTz);
        $start_time->setTimezone($londonTz);
        $hit->start = $start_time->format(DATE_ATOM);

        $end_time = new DateTime($hit->end, $utcTz);
        $end_time->setTimezone($londonTz);
        $hit->end = $end_time->format(DATE_ATOM);

       $event = entity_load_single('node', $hit->event_nid);
       if(isset($event->field_event_venue['und'])){
          
           $venue_tid = $event->field_event_venue['und'][0]['tid'];
           
           // we also tag the event with the start and end time for this period - i.e. those in the hit
           
           // special handling for all day events that are indicated by having identical start and end time of 00:00:00
           if($hit->start == $hit->end && preg_match('/T00:00:00$/', $hit->start)){
               $hit->end = substr($hit->end, 0, 10) . 'T23:59:59';
           }
                           
           // there is a chance that the venue has been deleted
           // but the events will still hang around 
           if(isset($venues[$venue_tid])){
               $venues[$venue_tid]->events[] = array($event, $hit);
              // we count the events at venues
              increment_event_count($venues[$venue_tid], $venues);
           }
           
       }
    }
    
    
    // we are not getting them in the right order so do another sort of 
    foreach(array_keys($venues) as $vtid){
        if(isset($venues[$vtid]->events)){
            usort($venues[$vtid]->events, 'sort_by_repeat');
        }
    }
    
}

function increment_event_count($venue, &$all_venues){
    
    if(isset($venue->event_count)){
        $venue->event_count++;
    }else{
        $venue->event_count = 1;
    }
    
    if($venue->parents[0] > 0){
        increment_event_count($all_venues[$venue->parents[0]], $all_venues);
    }
    
}

function sort_by_repeat($a, $b){
    
    $a_start = new DateTime($a[1]->start);
    $b_start = new DateTime($b[1]->start);
    
    if($a_start > $b_start) return 1;
    if($a_start == $b_start) return 0;
    if($a_start < $b_start) return -1;
    
}

function sort_by_weight($a, $b){
    if ($a->weight == $b->weight) {
            return 0;
        }
    return ($a->weight < $b->weight) ? -1 : 1;
}

