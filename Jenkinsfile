pipeline
{
    agent { 
        docker { image "ubuntu:latest" } 
    }
    
    stages
    {
        stage("Building")
        { 
            steps
            {
                echo "hello from docker"
                dir("/build")
                {
                    sh cmake ..
                    sh make 
                }
            }
        }
    }
 } // pipeline